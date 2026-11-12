// Replaces the grounded pouch in Enclosure Assembly with a constrained one that slides along the tunnel.
// Constraint "Pouch insertion" (parameter "Insertion"): 0 mm = fully inserted, larger = pulled out toward -X.
// Drive it in Inventor (right-click the constraint > Drive) to animate. Run with pouch_slide.ps1.
using System;
using System.Collections.Generic;
using System.Linq;
using Inventor;

public static class PouchSlide
{
    // Fully inserted placement, read from the pouch's placement in the freshly generated assembly (Enclosure.cs puts
    // its tip at TipX, centred on SensorY, back face on the tunnel floor at SlotFloorZ, body toward -X).
    static double TipX, SensorY, SlotFloorZ;

    static ComponentOccurrence Find(AssemblyDocument asm, string prefix)
    {
        foreach (ComponentOccurrence o in asm.ComponentDefinition.Occurrences) if (o.Name.StartsWith(prefix)) return o;
        return null;
    }

    static object Proxy(ComponentOccurrence occ, object entity) { object p; occ.CreateGeometryProxy(entity, out p); return p; }

    static string Box(ComponentOccurrence o)
    {
        var rb = o.RangeBox;
        return string.Format("x {0:F2}..{1:F2} y {2:F2}..{3:F2} z {4:F2}..{5:F2}", rb.MinPoint.X*10, rb.MaxPoint.X*10, rb.MinPoint.Y*10, rb.MaxPoint.Y*10, rb.MinPoint.Z*10, rb.MaxPoint.Z*10);
    }

    static bool Near(double a, double b) { return Math.Abs(a - b) < 0.02; }

    // Tries flush, then mate, keeping whichever leaves the checked coordinate where it should be.
    static AssemblyConstraint Place(AssemblyDocument asm, ComponentOccurrence pouch, object pouchPlane, object asmPlane, double offsetCm, Func<bool> ok, string name, List<string> log, bool positiveOnly = false)
    {
        var cons = asm.ComponentDefinition.Constraints;
        var tried = new List<string>();
        // positiveOnly also tries the entities the other way round, which flips the offset direction.
        foreach (string kind in positiveOnly ? new[] { "flush", "mate", "flush~", "mate~" } : new[] { "flush", "mate", "flush-", "mate-" })
        {
            double off = kind.EndsWith("-") ? -offsetCm : offsetCm;
            bool swap = kind.EndsWith("~");
            AssemblyConstraint c;
            try
            {
                object e1 = swap ? asmPlane : pouchPlane, e2 = swap ? pouchPlane : asmPlane;
                c = kind.StartsWith("flush") ? (AssemblyConstraint)cons.AddFlushConstraint(e1, e2, off)
                                             : (AssemblyConstraint)cons.AddMateConstraint(e1, e2, off);
            }
            catch (Exception e) { tried.Add(kind + ": rejected (" + e.Message + ")"); continue; }
            asm.Update();
            if (ok()) { c.Name = name; log.Add(name + ": " + kind); return c; }
            tried.Add(kind + ": " + Box(pouch) + " xx=" + pouch.Transformation.Cell[1, 1].ToString("F2"));
            c.Delete(); asm.Update();
        }
        throw new Exception("Could not place " + name + " | " + string.Join(" | ", tried) + " | so far: " + string.Join("; ", log));
    }

    public static string Run(object appObj)
    {
        var app = (Application)appObj; var log = new List<string>();
        var asm = (AssemblyDocument)app.ActiveDocument;
        var acd = asm.ComponentDefinition;
        var pouch = Find(asm, "MobileDetect Pouch");
        if (pouch == null) throw new Exception("No pouch occurrence in the active assembly");
        var pd = (PartComponentDefinition)pouch.Definition;
        if (!pouch.Grounded) throw new Exception("Run this on a freshly generated Enclosure Assembly (pouch still grounded in place)");
        var t0 = pouch.Transformation;
        TipX = t0.Cell[1, 4] * 10; SensorY = t0.Cell[2, 4] * 10; SlotFloorZ = t0.Cell[3, 4] * 10;
        // Remove anything a previous run left behind.
        foreach (AssemblyConstraint c in new List<AssemblyConstraint>(acd.Constraints.Cast<AssemblyConstraint>()))
            if (c.Name.StartsWith("Pouch ")) c.Delete();
        foreach (WorkPlane w in new List<WorkPlane>(acd.WorkPlanes.Cast<WorkPlane>()))
            if (w.Name == "Tunnel floor" || w.Name == "Tunnel centreline" || w.Name == "Pouch stop") w.Delete();
        asm.Update();
        // Start from the fully inserted placement so the solver only has to confirm it.
        var tg0 = app.TransientGeometry; var m0 = tg0.CreateMatrix();
        m0.SetCoordinateSystem(tg0.CreatePoint(TipX / 10, SensorY / 10, SlotFloorZ / 10), tg0.CreateVector(-1, 0, 0), tg0.CreateVector(0, -1, 0), tg0.CreateVector(0, 0, 1));
        pouch.Grounded = false;
        pouch.Transformation = m0;

        // Assembly reference planes: tunnel floor (Z), tunnel centreline (Y), fully-inserted stop (X).
        var tg = app.TransientGeometry;
        Func<double, double, double, double[], double[], string, WorkPlane> fixedPlane = (x, y, z, a, b, name) =>
        {
            var wp = acd.WorkPlanes.AddFixed(tg.CreatePoint(x / 10, y / 10, z / 10), tg.CreateUnitVector(a[0], a[1], a[2]), tg.CreateUnitVector(b[0], b[1], b[2]));
            wp.Name = name; wp.Visible = false; wp.Grounded = true;   // else the solver moves the plane, not the pouch
            return wp;
        };
        log.Add(string.Format("Fully inserted: tip X {0:F2}, centreline Y {1:F2}, floor Z {2:F2}", TipX, SensorY, SlotFloorZ));
        var floor = fixedPlane(0, 0, SlotFloorZ, new double[] { 1, 0, 0 }, new double[] { 0, 1, 0 }, "Tunnel floor");          // normal +Z
        var centre = fixedPlane(0, SensorY, 0, new double[] { 0, 0, 1 }, new double[] { 1, 0, 0 }, "Tunnel centreline");    // normal +Y
        var stop = fixedPlane(TipX, 0, 0, new double[] { 0, 0, 1 }, new double[] { 0, 1, 0 }, "Pouch stop");                // normal -X

        object pXY = Proxy(pouch, pd.WorkPlanes[3]), pXZ = Proxy(pouch, pd.WorkPlanes[2]), pYZ = Proxy(pouch, pd.WorkPlanes[1]);

        Place(asm, pouch, pXY, floor, 0, () => Near(pouch.RangeBox.MinPoint.Z * 10, SlotFloorZ), "Pouch on floor", log);
        // Centred, and turned so the body runs toward -X (pouch local +X maps to assembly -X).
        Place(asm, pouch, pXZ, centre, 0, () => Near((pouch.RangeBox.MinPoint.Y + pouch.RangeBox.MaxPoint.Y) * 5, SensorY)
                                                && pouch.Transformation.Cell[1, 1] < -0.99, "Pouch centred", log);
        // Insertion: offset +1 cm must move the tip 10 mm toward -X.
        var ins = Place(asm, pouch, pYZ, stop, 1.0, () => Near(pouch.RangeBox.MaxPoint.X * 10, TipX - 10), "Pouch insertion", log, true);
        Parameter p = ins is MateConstraint ? ((MateConstraint)ins).Offset : ((FlushConstraint)ins).Offset;
        p.Name = "Insertion";
        p.Expression = "0 mm";
        asm.Update();
        log.Add("At Insertion = 0: " + Box(pouch));

        // Sweep the travel and report any collision between the pouch and the base or optical module.
        var others = app.TransientObjects.CreateObjectCollection();
        foreach (ComponentOccurrence o in acd.Occurrences)
            if (o.Name.StartsWith("Enclosure Base") || o.Name.StartsWith("Optical Module") || o.Name.StartsWith("Test Sled")) others.Add(o);
        var one = app.TransientObjects.CreateObjectCollection(); one.Add(pouch);
        foreach (double mm in new double[] { 0, 5, 10, 20, 30, 45, 60, 75, 90, 100 })
        {
            p.Expression = mm.ToString(System.Globalization.CultureInfo.InvariantCulture) + " mm";
            asm.Update();
            var res = acd.AnalyzeInterference(one, others);
            double vol = 0; foreach (InterferenceResult r in res) vol += r.Volume * 1000;
            log.Add(string.Format("Insertion {0,5:F0} mm: tip X {1,7:F2}, collisions {2} ({3:F3} mm3)", mm, pouch.RangeBox.MaxPoint.X * 10, res.Count, vol));
        }
        p.Expression = "0 mm";
        asm.Update();

        // Let the pouch be dragged between fully inserted and fully out, staying where it's dropped.
        ConstraintLimits lim = ins is MateConstraint ? ((MateConstraint)ins).ConstraintLimits : ((FlushConstraint)ins).ConstraintLimits;
        lim.MaximumEnabled = true; lim.Maximum.Expression = "95 mm"; lim.Maximum.Name = "InsertionMax";
        lim.MinimumEnabled = true; lim.Minimum.Expression = "0 mm"; lim.Minimum.Name = "InsertionMin";
        lim.RestingPositionEnabled = false;
        asm.Update();
        log.Add("Drag limits: 0 to 95 mm (no snap-back)");
        asm.Save();
        return string.Join("\n", log);
    }
}
