// Generates Electronics/Wiring.ipt: insulated wires (cylinders + spheres at bends) on the perf board's top face,
// loose leads up to the LCD backpack, and solder joints. Run with build_wiring.ps1.
//
// Coordinates are millimetres in the Electronics Assembly frame (perf board lower-left corner, +Z up).
// Perf grid: HoleX(i) = 8.17 + 2.54 i, HoleY(j) = 5.79 + 2.54 j.
//   ESP32 (USB toward +Y) right column at col 10 (X 33.57): 3V3 row 23, GND row 22, D21 row 13, D22 row 10.
//   ESP32 left column at col 0 (X 8.17): VIN row 23.
//   TCS3448 header at col 22 (X 64.05): VIN row 9, GND row 10, SCL row 11, SDA row 12.
//   LCD pads at row 23: SCL col 26, SDA col 27, 5V col 28, GND col 29.
// Wires that cross run on a raised layer (L2, L3) so insulated wires stack instead of colliding.
using System;
using System.Collections.Generic;
using Inventor;

public static class Wiring
{
    const double R = 0.8;                         // 22 AWG insulated, ~1.6 mm OD
    const double L1 = 2.4, L2 = 4.2, L3 = 6.0;    // board top is Z 1.6

    static Application app; static TransientBRep tb; static TransientGeometry tg;
    static Point P(double x, double y, double z) { return tg.CreatePoint(x / 10, y / 10, z / 10); }
    static double[] V(double x, double y, double z) { return new[] { x, y, z }; }

    static SurfaceBody Tube(double[][] pts)
    {
        SurfaceBody body = null;
        for (int i = 0; i < pts.Length; i++)
        {
            var s = tb.CreateSolidSphere(P(pts[i][0], pts[i][1], pts[i][2]), R / 10);
            if (body == null) body = s; else tb.DoBoolean(body, s, BooleanTypeEnum.kBooleanTypeUnion);
            if (i == 0) continue;
            var a = pts[i - 1]; var b = pts[i];
            if (Math.Abs(a[0]-b[0]) + Math.Abs(a[1]-b[1]) + Math.Abs(a[2]-b[2]) < 1e-6) continue;
            tb.DoBoolean(body, tb.CreateSolidCylinderCone(P(a[0], a[1], a[2]), P(b[0], b[1], b[2]), R / 10, R / 10, R / 10), BooleanTypeEnum.kBooleanTypeUnion);
        }
        return body;
    }

    static SurfaceBody Blobs(double[][] xy)
    {
        SurfaceBody body = null;
        foreach (var p in xy)
        {
            var c = tb.CreateSolidCylinderCone(P(p[0], p[1], 1.6), P(p[0], p[1], 2.6), 0.11, 0.11, 0.05);
            if (body == null) body = c; else tb.DoBoolean(body, c, BooleanTypeEnum.kBooleanTypeUnion);
        }
        return body;
    }

    static Asset Find(string[] names)
    {
        foreach (string n in names) foreach (AssetLibrary lib in app.AssetLibraries) foreach (Asset a in lib.AppearanceAssets) if (a.DisplayName == n) return a;
        return null;
    }

    static void Add(PartDocument doc, SurfaceBody body, string name, string look, List<string> log)
    {
        var defs = doc.ComponentDefinition.Features.NonParametricBaseFeatures;
        var def = defs.CreateDefinition();
        var col = app.TransientObjects.CreateObjectCollection(); col.Add(body);
        def.BRepEntities = col; def.OutputType = BaseFeatureOutputTypeEnum.kSolidOutputType;
        var f = defs.AddByDefinition(def); f.Name = name;
        var src = Find(look.Split('|'));
        if (src != null)
        {
            Asset local = null;
            foreach (Asset d in doc.AppearanceAssets) if (d.DisplayName == src.DisplayName) local = d;
            if (local == null) local = src.CopyTo(doc, false);
            f.Appearance = local;
        }
        log.Add(name);
    }

    const string BLUE = "Blue|Plastic - Glossy (Blue)", YELLOW = "Yellow|Plastic - Glossy (Yellow)", RED = "Red|Plastic - Glossy (Red)",
                 BLACK = "Black|Smooth - Black", ORANGE = "Orange|Plastic - Glossy (Orange)|Red", METAL = "Aluminum - Polished|Steel - Satin";

    public static string Build(object appObj, string path)
    {
        app = (Application)appObj; tb = app.TransientBRep; tg = app.TransientGeometry;
        if (System.IO.File.Exists(path)) throw new Exception("Refusing to overwrite " + path);
        var log = new List<string>();
        string tpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kPartDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var doc = (PartDocument)app.Documents.Add(DocumentTypeEnum.kPartDocumentObject, tpl, false);

        // ESP32 to TCS3448, all on the board (layer 1)
        Add(doc, Tube(new[] { V(33.57,31.19,L1), V(61.51,31.19,L1), V(61.51,33.73,L1), V(64.05,33.73,L1) }), "SCL (D22 to TCS)", YELLOW, log);
        Add(doc, Tube(new[] { V(33.57,38.81,L1), V(61.51,38.81,L1), V(61.51,36.27,L1), V(64.05,36.27,L1) }), "SDA (D21 to TCS)", BLUE, log);
        Add(doc, Tube(new[] { V(33.57,64.21,L1), V(69.13,64.21,L1), V(69.13,28.65,L1), V(64.05,28.65,L1) }), "3V3 (ESP32 to TCS VIN)", RED, log);
        Add(doc, Tube(new[] { V(33.57,61.67,L1), V(66.59,61.67,L1), V(66.59,31.19,L1), V(64.05,31.19,L1) }), "GND (ESP32 to TCS)", BLACK, log);
        // 5V from ESP32 VIN to the LCD pad, along the board margin
        Add(doc, Tube(new[] { V(79.29,64.21,L1), V(79.29,66.75,L1), V(8.17,66.75,L1), V(8.17,64.21,L1) }), "5V (VIN to LCD pad)", ORANGE, log);
        // LCD pads to the TCS3448 pins, hopping over the 3V3 and GND runs
        Add(doc, Tube(new[] { V(74.21,64.21,L1), V(74.21,64.21,L2), V(74.21,33.73,L2), V(64.05,33.73,L2), V(64.05,33.73,L1) }), "SCL (LCD pad link)", YELLOW, log);
        Add(doc, Tube(new[] { V(76.75,64.21,L1), V(76.75,64.21,L3), V(76.75,36.27,L3), V(64.05,36.27,L3), V(64.05,36.27,L1) }), "SDA (LCD pad link)", BLUE, log);
        Add(doc, Tube(new[] { V(81.83,64.21,L1), V(81.83,31.19,L1), V(81.83,31.19,L2), V(64.05,31.19,L2), V(64.05,31.19,L1) }), "GND (LCD pad link)", BLACK, log);

        // Loose leads from the LCD backpack pins (tips at Y 76, Z 16.75) over the board edge to the pads.
        // Pin X (backpack order SCL, SDA, VCC, GND): 48.52, 51.06, 53.60, 56.14. Each lead's run has its own height.
        double zp = 16.75, yd = 77.0;   // clears the lid's locating lip (inner edge Y 78.1)
        var leads = new[] {
            new { Name = "LCD lead SCL", Px = 48.52, Pad = 74.21, Z = 9.0,  Look = YELLOW },
            new { Name = "LCD lead SDA", Px = 51.06, Pad = 76.75, Z = 11.0, Look = BLUE },
            new { Name = "LCD lead 5V",  Px = 53.60, Pad = 79.29, Z = 13.0, Look = ORANGE },
            new { Name = "LCD lead GND", Px = 56.14, Pad = 81.83, Z = 15.0, Look = BLACK } };
        foreach (var l in leads)
            Add(doc, Tube(new[] { V(l.Px,76,zp), V(l.Px,yd,zp), V(l.Px,yd,l.Z), V(l.Pad,yd,l.Z), V(l.Pad,yd,L2), V(l.Pad,64.21,L2), V(l.Pad,64.21,L1) }), l.Name, l.Look, log);

        Add(doc, Blobs(new[] {
            new[]{33.57,31.19}, new[]{33.57,38.81}, new[]{33.57,61.67}, new[]{33.57,64.21}, new[]{8.17,64.21},
            new[]{64.05,28.65}, new[]{64.05,31.19}, new[]{64.05,33.73}, new[]{64.05,36.27},
            new[]{74.21,64.21}, new[]{76.75,64.21}, new[]{79.29,64.21}, new[]{81.83,64.21} }), "Solder joints", METAL, log);

        doc.PropertySets["Design Tracking Properties"]["Description"].Value = "Point-to-point wiring: I2C (D21 SDA, D22 SCL) to TCS3448 and LCD, 3V3, 5V, GND";
        doc.SaveAs(path, false);
        var rb = doc.ComponentDefinition.RangeBox;
        log.Add(string.Format("range x {0:F2}..{1:F2} y {2:F2}..{3:F2} z {4:F2}..{5:F2}", rb.MinPoint.X*10, rb.MaxPoint.X*10, rb.MinPoint.Y*10, rb.MaxPoint.Y*10, rb.MinPoint.Z*10, rb.MaxPoint.Z*10));
        doc.Close(true);
        return string.Join("\n", log);
    }

    static string Net(string name)
    {
        if (name.StartsWith("LCD lead ")) name = name.Substring(9);
        return name.StartsWith("Solder") ? null : name.Split(' ')[0];
    }

    // Overlap between wires of different nets (insulation would collide).
    public static string Check(object appObj, string path)
    {
        app = (Application)appObj; tb = app.TransientBRep;
        var doc = (PartDocument)app.Documents.Open(path, false);
        var items = new List<Tuple<string, string, SurfaceBody>>();
        foreach (PartFeature f in doc.ComponentDefinition.Features.NonParametricBaseFeatures)
        {
            string net = Net(f.Name); if (net == null) continue;
            foreach (SurfaceBody b in f.SurfaceBodies) items.Add(Tuple.Create(f.Name, net, b));
        }
        var log = new List<string>();
        for (int i = 0; i < items.Count; i++)
            for (int j = i + 1; j < items.Count; j++)
            {
                if (items[i].Item2 == items[j].Item2) continue;
                var a = tb.Copy(items[i].Item3); var b = tb.Copy(items[j].Item3);
                tb.DoBoolean(a, b, BooleanTypeEnum.kBooleanTypeIntersect);
                double vol = 0; try { vol = a.Volume[0.0001] * 1000; } catch { }
                if (vol > 1e-4) log.Add(string.Format("OVERLAP {0} x {1}: {2:F3} mm3", items[i].Item1, items[j].Item1, vol));
            }
        doc.Close(true);
        return log.Count == 0 ? "No overlaps between different nets" : string.Join("\n", log);
    }
}
