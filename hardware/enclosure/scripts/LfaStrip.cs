// Builds a bare LFA test strip model and "LFA Fit Check.iam" (Electronics Assembly + the strip), with the strip
// constrained to slide along its length under the two sensors. Run with build_lfa_fit_check.ps1. AddTo() adds the
// same sliding strip to the active assembly (build_enclosure.ps1 uses it for Enclosure Assembly).
//
// Strip (measured): 75 mm long, 1/8 in (3.175 mm) wide, bare. From the testing end: C line at 10 mm, T line at
// 20 mm, both 1 mm wide. Thickness is assumed (0.5 mm).
// Assembly frame: perf board lower-left corner, +Z up. Insert from -Y, testing end toward +Y.
// Photo-layout coordinates are shared with the enclosure through AsBuiltLayout.
using System;
using System.Collections.Generic;
using System.Linq;
using Inventor;

public static class LfaStrip
{
    const double Len = AsBuiltLayout.StripLen, Width = AsBuiltLayout.StripW, Thick = AsBuiltLayout.StripT;
    const double LineC = AsBuiltLayout.LineC, LineT = AsBuiltLayout.LineT, LineW = 1.0;
    const double SensorX = AsBuiltLayout.SensorX, TcsY = AsBuiltLayout.TcsY, AsY = AsBuiltLayout.AsY;
    const double StripTopZ = AsBuiltLayout.StripTopZ;
    const double TravelMax = 80.0;

    static Application app; static TransientGeometry tg;

    static double StopY { get { return AsBuiltLayout.StripStopY; } }

    static object Proxy(ComponentOccurrence occ, object entity) { object p; occ.CreateGeometryProxy(entity, out p); return p; }
    static bool Near(double a, double b) { return Math.Abs(a - b) < 0.02; }

    static Asset Find(string[] names)
    {
        foreach (string n in names) foreach (AssetLibrary lib in app.AssetLibraries) foreach (Asset a in lib.AppearanceAssets) if (a.DisplayName == n) return a;
        return null;
    }
    static void Look(PartDocument doc, ExtrudeFeature f, string looks)
    {
        var src = Find(looks.Split('|')); if (src == null) return;
        Asset local = null; foreach (Asset d in doc.AppearanceAssets) if (d.DisplayName == src.DisplayName) local = d;
        if (local == null) local = src.CopyTo(doc, false);
        f.Appearance = local;
    }

    static ExtrudeFeature Box(PartComponentDefinition cd, double x0, double y0, double x1, double y1, double z0, double h, string name)
    {
        object plane = cd.WorkPlanes[3];
        if (z0 != 0) { var wp = cd.WorkPlanes.AddByPlaneAndOffset(cd.WorkPlanes[3], z0 / 10); wp.Visible = false; plane = wp; }
        var sk = cd.Sketches.Add(plane);
        sk.SketchLines.AddAsTwoPointRectangle(tg.CreatePoint2d(x0 / 10, y0 / 10), tg.CreatePoint2d(x1 / 10, y1 / 10));
        var def = cd.Features.ExtrudeFeatures.CreateExtrudeDefinition(sk.Profiles.AddForSolid(), PartFeatureOperationEnum.kJoinOperation);
        def.SetDistanceExtent(h / 10, PartFeatureExtentDirectionEnum.kPositiveExtentDirection);
        var f = cd.Features.ExtrudeFeatures.Add(def); f.Name = name; sk.Visible = false;
        return f;
    }

    // Local frame: testing end at x = 0, strip along +x, centred on y = 0, back face z = 0, front (line) face z = Thick.
    static void BuildStrip(string path)
    {
        string tpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kPartDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var doc = (PartDocument)app.Documents.Add(DocumentTypeEnum.kPartDocumentObject, tpl, false);
        var cd = doc.ComponentDefinition;
        var body = Box(cd, 0, -Width / 2, Len, Width / 2, 0, Thick, "Strip");
        var c = Box(cd, LineC - LineW / 2, -Width / 2, LineC + LineW / 2, Width / 2, Thick, 0.05, "C line");
        var t = Box(cd, LineT - LineW / 2, -Width / 2, LineT + LineW / 2, Width / 2, Thick, 0.05, "T line");
        Look(doc, body, "White|Smooth - White");
        Look(doc, c, "Red|Plastic - Glossy (Red)");
        Look(doc, t, "Red|Plastic - Glossy (Red)");
        doc.PropertySets["Design Tracking Properties"]["Description"].Value =
            "Bare LFA strip, 75 x 3.175 mm; C line 10 mm and T line 20 mm from the testing end, 1 mm wide; thickness assumed 0.5 mm";
        doc.SaveAs(path, false);
        doc.Close(true);
    }

    static AssemblyConstraint Place(AssemblyDocument asm, object stripPlane, object asmPlane, double offsetCm, Func<bool> ok, string name, List<string> log, bool positiveOnly = false)
    {
        var cons = asm.ComponentDefinition.Constraints;
        var kinds = positiveOnly ? new[] { "flush", "mate", "flush~", "mate~" } : new[] { "flush", "mate", "flush-", "mate-" };
        foreach (string kind in kinds)
        {
            double off = kind.EndsWith("-") ? -offsetCm : offsetCm;
            bool swap = kind.EndsWith("~");
            object e1 = swap ? asmPlane : stripPlane, e2 = swap ? stripPlane : asmPlane;
            AssemblyConstraint c;
            try { c = kind.StartsWith("flush") ? (AssemblyConstraint)cons.AddFlushConstraint(e1, e2, off) : (AssemblyConstraint)cons.AddMateConstraint(e1, e2, off); }
            catch { continue; }
            asm.Update();
            if (ok()) { c.Name = name; log.Add(name + ": " + kind); return c; }
            c.Delete(); asm.Update();
        }
        throw new Exception("Could not place " + name + "; so far: " + string.Join("; ", log));
    }

    public static string Run(object appObj, string root)
    {
        app = (Application)appObj; tg = app.TransientGeometry; var log = new List<string>();
        string stripDir = System.IO.Path.Combine(root, "Test Strips");
        System.IO.Directory.CreateDirectory(stripDir);
        string stripPath = System.IO.Path.Combine(stripDir, "LFA Strip (bare).ipt");
        string asmPath = System.IO.Path.Combine(root, "LFA Fit Check.iam");
        if (System.IO.File.Exists(asmPath)) throw new Exception("Refusing to overwrite " + asmPath);
        if (!System.IO.File.Exists(stripPath)) BuildStrip(stripPath);

        string atpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kAssemblyDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var asm = (AssemblyDocument)app.Documents.Add(DocumentTypeEnum.kAssemblyDocumentObject, atpl, true);
        asm.ComponentDefinition.Occurrences.Add(System.IO.Path.Combine(root, "Electronics Assembly.iam"), tg.CreateMatrix()).Grounded = true;
        Attach(asm, stripPath, log);
        asm.PropertySets["Design Tracking Properties"]["Description"].Value = "Fit check: LFA strip sliding under the TCS3448 and AS7343";
        asm.SaveAs(asmPath, false);
        return string.Join("\n", log);
    }

    // Adds the sliding strip to the active assembly (e.g. Enclosure Assembly) and saves it.
    public static string AddTo(object appObj, string root)
    {
        app = (Application)appObj; tg = app.TransientGeometry; var log = new List<string>();
        string stripPath = System.IO.Path.Combine(root, "Test Strips", "LFA Strip (bare).ipt");
        if (!System.IO.File.Exists(stripPath)) { System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(stripPath)); BuildStrip(stripPath); }
        var asm = (AssemblyDocument)app.ActiveDocument;
        foreach (ComponentOccurrence o in asm.ComponentDefinition.Occurrences)
            if (o.Name.StartsWith("LFA Strip")) throw new Exception("The active assembly already has an LFA strip");
        Attach(asm, stripPath, log);
        asm.Save();
        return string.Join("\n", log);
    }

    static void Attach(AssemblyDocument asm, string stripPath, List<string> log)
    {
        var acd = asm.ComponentDefinition;
        // Start fully inserted: strip local x -> -Y, local y -> +X, local z -> +Z.
        var m = tg.CreateMatrix();
        m.SetCoordinateSystem(tg.CreatePoint(SensorX / 10, StopY / 10, (StripTopZ - Thick) / 10),
                              tg.CreateVector(0, -1, 0), tg.CreateVector(1, 0, 0), tg.CreateVector(0, 0, 1));
        var strip = acd.Occurrences.Add(stripPath, m);
        var pd = (PartComponentDefinition)strip.Definition;

        Func<double, double, double, double[], double[], string, WorkPlane> fixedPlane = (x, y, z, a, b, name) =>
        {
            var wp = acd.WorkPlanes.AddFixed(tg.CreatePoint(x / 10, y / 10, z / 10), tg.CreateUnitVector(a[0], a[1], a[2]), tg.CreateUnitVector(b[0], b[1], b[2]));
            wp.Name = name; wp.Visible = false; wp.Grounded = true;   // else the solver moves the plane, not the strip
            return wp;
        };
        var floor = fixedPlane(0, 0, StripTopZ - Thick, new double[] { 1, 0, 0 }, new double[] { 0, 1, 0 }, "Strip floor");      // normal +Z
        var centre = fixedPlane(SensorX, 0, 0, new double[] { 0, 1, 0 }, new double[] { 0, 0, 1 }, "Strip centreline");         // normal +X
        var stop = fixedPlane(0, StopY, 0, new double[] { 0, 0, 1 }, new double[] { 1, 0, 0 }, "Strip stop");                     // normal +Y

        object sXY = Proxy(strip, pd.WorkPlanes[3]), sXZ = Proxy(strip, pd.WorkPlanes[2]), sYZ = Proxy(strip, pd.WorkPlanes[1]);
        Func<double> stripTipY = () => strip.RangeBox.MaxPoint.Y * 10;
        Place(asm, sXY, floor, 0, () => Near(strip.RangeBox.MinPoint.Z * 10, StripTopZ - Thick), "Strip on floor", log);
        Place(asm, sXZ, centre, 0, () => Near((strip.RangeBox.MinPoint.X + strip.RangeBox.MaxPoint.X) * 5, SensorX)
                                          && strip.Transformation.Cell[2, 1] < -0.99, "Strip centred", log);   // local x -> -Y
        var ins = Place(asm, sYZ, stop, 1.0, () => Near(stripTipY(), StopY - 10), "Strip insertion", log, true);
        Parameter p = ins is MateConstraint ? ((MateConstraint)ins).Offset : ((FlushConstraint)ins).Offset;
        p.Name = "StripInsertion"; p.Expression = "0 mm"; asm.Update();

        double tY = StopY - LineT, cY = StopY - LineC;
        log.Add(string.Format("Fully inserted: testing end Y {0:F2}; C line Y {1:F2} ({2:F2} mm from AS7343); T line Y {3:F2} ({4:F2} mm from TCS3448); far end Y {5:F2}",
            StopY, cY, AsY - cY, tY, TcsY - tY, StopY - Len));
        var rb = strip.RangeBox;
        log.Add(string.Format("Strip: x {0:F2}..{1:F2} y {2:F2}..{3:F2} z {4:F2}..{5:F2}", rb.MinPoint.X*10, rb.MaxPoint.X*10, rb.MinPoint.Y*10, rb.MaxPoint.Y*10, rb.MinPoint.Z*10, rb.MaxPoint.Z*10));

        // Collisions along the strip's travel with the enclosure, if there is one in this assembly.
        var others = app.TransientObjects.CreateObjectCollection();
        foreach (ComponentOccurrence o in acd.Occurrences)
            if (o.Name.StartsWith("Enclosure Base") || o.Name.StartsWith("Optical Module") || o.Name.StartsWith("Enclosure Lid") || o.Name.StartsWith("Test Sled")) others.Add(o);
        if (others.Count > 0)
        {
            var one = app.TransientObjects.CreateObjectCollection(); one.Add(strip);
            foreach (double mm in new double[] { 0, 10, 20, 40, 60, 80 })
            {
                p.Expression = mm.ToString(System.Globalization.CultureInfo.InvariantCulture) + " mm"; asm.Update();
                var res = acd.AnalyzeInterference(one, others);
                double vol = 0; foreach (InterferenceResult r in res) vol += r.Volume * 1000;
                log.Add(string.Format("Strip insertion {0,3:F0} mm: testing end Y {1,6:F2}, collisions {2} ({3:F3} mm3)", mm, stripTipY(), res.Count, vol));
                if (vol > 1e-4) throw new Exception("Strip collides with enclosure: " + log[log.Count - 1]);
            }
            p.Expression = "0 mm"; asm.Update();
        }

        // Then let it be dragged between fully inserted and fully out (with limits on, the offset no longer drives it).
        ConstraintLimits lim = ins is MateConstraint ? ((MateConstraint)ins).ConstraintLimits : ((FlushConstraint)ins).ConstraintLimits;
        lim.MaximumEnabled = true; lim.Maximum.Expression = TravelMax + " mm"; lim.Maximum.Name = "StripInsertionMax";
        lim.MinimumEnabled = true; lim.Minimum.Expression = "0 mm"; lim.Minimum.Name = "StripInsertionMin";
        lim.RestingPositionEnabled = false;
        asm.Update();
    }
}
