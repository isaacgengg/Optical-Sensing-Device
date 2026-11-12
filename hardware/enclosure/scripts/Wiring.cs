// Generates Electronics/Wiring.ipt: insulated wires (cylinders + spheres at bends) on the perf board's top face,
// loose leads up to the LCD backpack, and solder joints. Run with build_wiring.ps1 (after build_electronics.ps1).
//
// Coordinates are millimetres in the Electronics Assembly frame (perf board lower-left corner, +Z up).
// Perf grid: HoleX(i) = 8.17 + 2.54 i, HoleY(j) = 5.79 + 2.54 j. Holes are written (column, row).
//   ESP32 right column, col 11 (X 36.11): 3V3 row 21, GND row 20, D4 row 17, D21 row 11, D22 row 8.
//   ESP32 left column, col 1 (X 10.71): VIN row 21, GND row 20.
//   TCS3448 header, row 9 (Y 28.65), and AS7343 header, row 2 (Y 10.87), both cols 22-27:
//     INT col 22, GPIO col 23, SDA col 24, SCL col 25, GND col 26, VIN col 27.
//   LCD pads, row 23 (Y 64.21): SCL col 16, SDA col 17, 5V col 18, GND col 19.
// Colours follow the real board: SDA orange, SCL yellow, 3V3 red, GND black; 5V white.
// Wires that cross run on raised layers (L2, L3) so insulated wires stack instead of colliding.
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
    static double HX(int i) { return 8.17 + 2.54 * i; }
    static double HY(int j) { return 5.79 + 2.54 * j; }

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

    static SurfaceBody Blobs(List<double[]> xy)
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

    const string ORANGE = "Orange|Plastic - Glossy (Orange)", YELLOW = "Yellow|Plastic - Glossy (Yellow)", RED = "Red|Plastic - Glossy (Red)",
                 BLACK = "Black|Smooth - Black", WHITE = "White|Smooth - White", METAL = "Aluminum - Polished|Steel - Satin";

    public static string Build(object appObj, string path)
    {
        app = (Application)appObj; tb = app.TransientBRep; tg = app.TransientGeometry;
        if (System.IO.File.Exists(path)) throw new Exception("Refusing to overwrite " + path);
        var log = new List<string>();
        string tpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kPartDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var doc = (PartDocument)app.Documents.Add(DocumentTypeEnum.kPartDocumentObject, tpl, false);

        double ex = HX(11), vin = HX(1);
        double e3v3 = HY(21), eGnd = HY(20), eSda = HY(11), eScl = HY(8);
        double tRow = HY(9), aRow = HY(2);
        double cSda = HX(24), cScl = HX(25), cGnd = HX(26), cVin = HX(27);
        // Lanes above the TCS header (rows 10-13): SDA lowest, then SCL, GND, VIN, so each lane ends before the next
        // one turns down onto its pin.
        double lSda = HY(10), lScl = HY(11), lGnd = HY(12), lVin = HY(13);
        // Risers just right of the ESP32's right-hand column.
        double rSda = HX(12), rScl = HX(13), rGnd = HX(14), r3v3 = HX(15);

        // ESP32 to TCS3448, all on layer 1 except SCL, which hops over the SDA lane on layer 2.
        Add(doc, Tube(new[] { V(ex,eSda,L1), V(rSda,eSda,L1), V(rSda,lSda,L1), V(cSda,lSda,L1), V(cSda,tRow,L1) }), "SDA (D21 to TCS)", ORANGE, log);
        Add(doc, Tube(new[] { V(ex,eScl,L1), V(rScl,eScl,L1), V(rScl,eScl,L2), V(rScl,lScl,L2), V(cScl,lScl,L2), V(cScl,tRow,L2), V(cScl,tRow,L1) }), "SCL (D22 to TCS)", YELLOW, log);
        Add(doc, Tube(new[] { V(ex,eGnd,L1), V(rGnd,eGnd,L1), V(rGnd,lGnd,L1), V(cGnd,lGnd,L1), V(cGnd,tRow,L1) }), "GND (ESP32 to TCS)", BLACK, log);
        Add(doc, Tube(new[] { V(ex,e3v3,L1), V(r3v3,e3v3,L1), V(r3v3,lVin,L1), V(cVin,lVin,L1), V(cVin,tRow,L1) }), "3V3 (ESP32 to TCS VIN)", RED, log);

        // TCS3448 to AS7343: four straight bridges down the columns (same pin order on both headers).
        Add(doc, Tube(new[] { V(cVin,tRow,L1), V(cVin,aRow,L1) }), "3V3 (TCS to AS bridge)", RED, log);
        Add(doc, Tube(new[] { V(cGnd,tRow,L1), V(cGnd,aRow,L1) }), "GND (TCS to AS bridge)", BLACK, log);
        Add(doc, Tube(new[] { V(cScl,tRow,L1), V(cScl,aRow,L1) }), "SCL (TCS to AS bridge)", YELLOW, log);
        Add(doc, Tube(new[] { V(cSda,tRow,L1), V(cSda,aRow,L1) }), "SDA (TCS to AS bridge)", ORANGE, log);

        // LCD pads (row 23) onto the bus lanes, and 5V from ESP32 VIN along the top margin.
        double pRow = HY(23), pScl = HX(16), pSda = HX(17), p5v = HX(18), pGnd = HX(19);
        // Each link drops onto its own lane, hopping over the lanes it crosses (VIN, GND on layer 1; SCL on layer 2).
        Add(doc, Tube(new[] { V(pSda,pRow,L1), V(pSda,pRow,L3), V(pSda,lSda,L3), V(pSda,lSda,L1) }), "SDA (LCD pad link)", ORANGE, log);
        Add(doc, Tube(new[] { V(pScl,pRow,L1), V(pScl,pRow,L2), V(pScl,lScl,L2) }), "SCL (LCD pad link)", YELLOW, log);
        Add(doc, Tube(new[] { V(pGnd,pRow,L1), V(pGnd,pRow,L2), V(pGnd,lGnd,L2), V(pGnd,lGnd,L1) }), "GND (LCD pad link)", BLACK, log);
        double margin = pRow + 2.54;
        Add(doc, Tube(new[] { V(p5v,pRow,L1), V(p5v,margin,L1), V(vin,margin,L1), V(vin,e3v3,L1) }), "5V (VIN to LCD pad)", WHITE, log);

        // Loose leads from the LCD backpack pins (tips at Y 76, Z 16.75; order SCL, SDA, VCC, GND) over the board
        // edge to the pads, crossing the 5V margin wire on layer 2.
        double zp = 16.75, yd = 77.0;
        var leads = new[] {
            new { Name = "LCD lead SCL", Px = 48.52, Pad = pScl, Look = YELLOW },
            new { Name = "LCD lead SDA", Px = 51.06, Pad = pSda, Look = ORANGE },
            new { Name = "LCD lead 5V",  Px = 53.60, Pad = p5v,  Look = WHITE },
            new { Name = "LCD lead GND", Px = 56.14, Pad = pGnd, Look = BLACK } };
        foreach (var l in leads)
            Add(doc, Tube(new[] { V(l.Px,76,zp), V(l.Px,yd,zp), V(l.Pad,yd,L2), V(l.Pad,pRow,L2), V(l.Pad,pRow,L1) }), l.Name, l.Look, log);

        var joints = new List<double[]> {
            new[]{ex,e3v3}, new[]{ex,eGnd}, new[]{ex,eSda}, new[]{ex,eScl}, new[]{vin,e3v3},
            new[]{pScl,pRow}, new[]{pSda,pRow}, new[]{p5v,pRow}, new[]{pGnd,pRow} };
        foreach (double c in new[] { cVin, cGnd, cScl, cSda }) { joints.Add(new[] { c, tRow }); joints.Add(new[] { c, aRow }); }
        Add(doc, Blobs(joints), "Solder joints", METAL, log);

        doc.PropertySets["Design Tracking Properties"]["Description"].Value = "Point-to-point wiring: I2C (D21 SDA, D22 SCL) to TCS3448, AS7343 and LCD; 3V3, 5V, GND";
        doc.SaveAs(path, false);
        var rb = doc.ComponentDefinition.RangeBox;
        log.Add(string.Format("range x {0:F2}..{1:F2} y {2:F2}..{3:F2} z {4:F2}..{5:F2}", rb.MinPoint.X*10, rb.MaxPoint.X*10, rb.MinPoint.Y*10, rb.MaxPoint.Y*10, rb.MinPoint.Z*10, rb.MaxPoint.Z*10));
        doc.Close(true);
        return string.Join("\n", log);
    }

    static string Net(string name)
    {
        if (name.StartsWith("LCD lead ")) name = name.Substring(9);
        if (name.StartsWith("Solder")) return null;
        string n = name.Split(' ')[0];
        return n == "5V" ? "5V" : n;
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
