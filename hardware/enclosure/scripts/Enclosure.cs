// Generates the enclosure parts in Inventor 2027: Enclosure Base, Enclosure Lid, the approximate MobileDetect
// pouch (only if missing), and Enclosure Assembly (wrapping Electronics Assembly). Run with build_enclosure.ps1.
//
// All numbers are millimetres in the Electronics Assembly frame: origin at the perf board's lower-left corner,
// +Z up, perf board bottom at Z = 0. The pouch enters through the -X wall (under the ESP32), tip first, and
// stops in a tapered pocket with its pad under the TCS3448. Change the constants and re-run to regenerate.
using System;
using System.Collections.Generic;
using Inventor;

public static class Enclosure
{
    // ---------- Fixed by the electronics assembly ----------
    const double SensorX = 70.02, SensorY = 35.00, SensorFaceZ = -4.75;
    const double TcsX0 = 61.51, TcsX1 = 79.29, TcsY0 = 22.30, TcsY1 = 47.70;   // breakout outline
    const double TcsLowestZ = -6.55;                    // STEMMA QT connectors
    const double UsbX = 20.87, UsbZ = -5.40;            // ESP32 micro-USB centre, facing +Y
    const double LcdPcbTopZ = 22.1, LcdTopZ = 29.3;
    // LCD rotated 90 deg: bezel and M3 holes in assembly coordinates
    const double BezelX0 = 32.65, BezelX1 = 56.85, BezelY0 = -0.6, BezelY1 = 70.6;
    static readonly double[][] LcdHoles = { new[]{29.25,72.5}, new[]{29.25,-2.5}, new[]{60.25,72.5}, new[]{60.25,-2.5} };

    // ---------- Pouch (product image scaled to DetectaChem's 1.98 x 3.54 in) ----------
    // Half-width of the pouch at distance s from its bottom tip.
    static readonly double[][] PouchHalf = {
        new[]{0,9.5}, new[]{0.95,10.0}, new[]{5.2,12.97}, new[]{9.4,15.46}, new[]{13.7,17.95}, new[]{17.9,20.44},
        new[]{22.1,22.5}, new[]{26.4,23.77}, new[]{30.6,24.94}, new[]{34.8,25.15}, new[]{77.2,25.15},
        new[]{81.4,23.6}, new[]{85.7,19.3}, new[]{89.8,15.0} };
    const double PadS0 = 2.6, PadS1 = 20.3, PadHalf = 9.1, PadCentreS = 11.45;
    const double PouchT = 5.0;     // ASSUMED thickness at the pad end; measure
    const double Clr = 0.5;        // clearance around the pouch outline
    const double Rail = 2.0;       // tunnel wall thickness

    // ---------- Optics ----------
    const double SensorToPad = 20.0;
    const double ApertureD = 3.0;
    const double WindowW = 16.0;
    const double ChamberHalfY = 10.85, ChamberHalfX = 9.0;   // inner half-sizes; Y sets the 45 deg LED geometry
    const double LedD = 5.2;            // bore for 5 mm LEDs
    const double SleeveD = 8.0;         // outside of the LED sleeve
    const double SleeveLen = 5.0;       // sleeve length outside the chamber wall, along the LED axis
    const double DiffuserD = 10.0, DiffuserDepth = 1.0;   // recess for a diffuser disc on the inner wall

    // ---------- Shell ----------
    const double Wall = 2.0, Floor = 2.0, Plate = 1.5;
    const double InX0 = -1.5, InX1 = 91.0, InY0 = -6.0, InY1 = 79.5;
    const double SplitZ = 4.0;
    const double SlotH = 9.0;
    const double PocketClr = 0.5, PocketTopGap = 0.2;

    // Derived
    static double PadZ { get { return SensorFaceZ - SensorToPad; } }
    static double SlotFloorZ { get { return PadZ - PouchT; } }
    static double SlotCeilZ { get { return SlotFloorZ + SlotH; } }
    static double PlateTopZ { get { return TcsLowestZ - 0.45; } }      // chamber ceiling plate, top face
    static double TipX { get { return SensorX + PadCentreS; } }
    static double OutX0 { get { return InX0 - Wall; } }
    static double OutX1 { get { return InX1 + Wall; } }
    static double OutY0 { get { return InY0 - Wall; } }
    static double OutY1 { get { return InY1 + Wall; } }
    static double BaseBottomZ { get { return SlotFloorZ - Floor; } }
    // Optical module footprint (= opening in the base): breakout outline + pocket clearance + wall.
    const double ModFit = 0.2;          // gap between the module and its opening, each side
    static double PocketX0 { get { return TcsX0 - PocketClr; } }
    static double PocketX1 { get { return TcsX1 + PocketClr; } }
    static double PocketY0 { get { return TcsY0 - PocketClr; } }
    static double PocketY1 { get { return TcsY1 + PocketClr; } }
    static double ModX0 { get { return PocketX0 - Wall - ModFit; } }
    static double ModX1 { get { return PocketX1 + Wall + ModFit; } }
    static double ModY0 { get { return PocketY0 - Wall - ModFit; } }
    static double ModY1 { get { return PocketY1 + Wall + ModFit; } }

    static Application app; static TransientGeometry tg;
    static PartComponentDefinition cd;
    static List<string> log;

    static Point2d P2(double x, double y) { return tg.CreatePoint2d(x / 10, y / 10); }
    static Point P3(double x, double y, double z) { return tg.CreatePoint(x / 10, y / 10, z / 10); }

    static object Plane(double z)
    {
        if (z == 0) return cd.WorkPlanes[3];
        var wp = cd.WorkPlanes.AddByPlaneAndOffset(cd.WorkPlanes[3], z / 10);
        wp.Visible = false;
        return wp;
    }

    static ExtrudeFeature Extrude(PlanarSketch sk, double h, PartFeatureOperationEnum op, string name)
    {
        var def = cd.Features.ExtrudeFeatures.CreateExtrudeDefinition(sk.Profiles.AddForSolid(), op);
        def.SetDistanceExtent(Math.Abs(h) / 10, h >= 0 ? PartFeatureExtentDirectionEnum.kPositiveExtentDirection : PartFeatureExtentDirectionEnum.kNegativeExtentDirection);
        var f = cd.Features.ExtrudeFeatures.Add(def);
        f.Name = name; sk.Visible = false;
        return f;
    }

    static ExtrudeFeature Rects(double z, double h, double[][] rects, PartFeatureOperationEnum op, string name)
    {
        var sk = cd.Sketches.Add(Plane(z));
        foreach (var r in rects) sk.SketchLines.AddAsTwoPointRectangle(P2(r[0], r[1]), P2(r[2], r[3]));
        return Extrude(sk, h, op, name);
    }
    static ExtrudeFeature Rect(double z, double h, double x0, double y0, double x1, double y1, PartFeatureOperationEnum op, string name)
    { return Rects(z, h, new[] { new[] { x0, y0, x1, y1 } }, op, name); }

    static ExtrudeFeature Poly(double z, double h, List<double[]> pts, PartFeatureOperationEnum op, string name)
    {
        var sk = cd.Sketches.Add(Plane(z));
        SketchLine first = null, prev = null;
        for (int i = 1; i < pts.Count; i++)
        {
            SketchLine l = prev == null
                ? sk.SketchLines.AddByTwoPoints(P2(pts[0][0], pts[0][1]), P2(pts[1][0], pts[1][1]))
                : sk.SketchLines.AddByTwoPoints(prev.EndSketchPoint, P2(pts[i][0], pts[i][1]));
            if (first == null) first = l;
            prev = l;
        }
        sk.SketchLines.AddByTwoPoints(prev.EndSketchPoint, first.StartSketchPoint);
        return Extrude(sk, h, op, name);
    }

    static ExtrudeFeature Circles(double z, double h, double[][] centres, double r, PartFeatureOperationEnum op, string name)
    {
        var sk = cd.Sketches.Add(Plane(z));
        foreach (var c in centres) sk.SketchCircles.AddByCenterRadius(P2(c[0], c[1]), r / 10);
        return Extrude(sk, h, op, name);
    }

    // Cylinder of diameter d centred on point c, along direction u, total length len (symmetric about c).
    static void CylinderAlong(double[] c, double[] u, double d, double len, PartFeatureOperationEnum op, string name)
    {
        var sk3 = cd.Sketches3D.Add();
        var line = sk3.SketchLines3D.AddByTwoPoints(P3(c[0], c[1], c[2]), P3(c[0] + u[0], c[1] + u[1], c[2] + u[2]));
        sk3.Visible = false;
        var wp = cd.WorkPlanes.AddByNormalToCurve(line, line.StartSketchPoint);
        wp.Visible = false;
        var sk = cd.Sketches.Add(wp);
        var centre = (SketchPoint)sk.AddByProjectingEntity(line.StartSketchPoint);
        sk.SketchCircles.AddByCenterRadius(centre, d / 20);
        var def = cd.Features.ExtrudeFeatures.CreateExtrudeDefinition(sk.Profiles.AddForSolid(), op);
        def.SetDistanceExtent(len / 10, PartFeatureExtentDirectionEnum.kSymmetricExtentDirection);
        var f = cd.Features.ExtrudeFeatures.Add(def);
        f.Name = name; sk.Visible = false;
    }

    // Pouch outline offset outward by c, tip at tipX, body running in direction dir (+1 = +X, -1 = -X),
    // centred on y = cy, cut off at x = xEnd (NaN = whole pouch).
    // slideOut = true never narrows away from the tip, so the whole pouch can slide out through it.
    static List<double[]> Outline(double tipX, double dir, double cy, double c, double xEnd, bool slideOut = false)
    {
        var side = new List<double[]>();
        double widest = 0;
        foreach (var p in PouchHalf)
        {
            double x = tipX + dir * p[0];
            if (!double.IsNaN(xEnd) && dir * (x - xEnd) > 0) break;
            widest = Math.Max(widest, p[1]);
            side.Add(new[] { x, cy - ((slideOut ? widest : p[1]) + c) });
        }
        side[0] = new[] { tipX - dir * c, cy - (PouchHalf[0][1] + c) };
        if (!double.IsNaN(xEnd)) side.Add(new[] { xEnd, side[side.Count - 1][1] });
        var pts = new List<double[]>(side);
        for (int i = side.Count - 1; i >= 0; i--) pts.Add(new[] { side[i][0], 2 * cy - side[i][1] });
        return pts;
    }

    static PartDocument NewPart(string path)
    {
        if (System.IO.File.Exists(path)) throw new Exception("Refusing to overwrite " + path);
        string tpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kPartDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var doc = (PartDocument)app.Documents.Add(DocumentTypeEnum.kPartDocumentObject, tpl, false);
        cd = doc.ComponentDefinition;
        return doc;
    }

    static Asset Find(string[] names)
    {
        foreach (string n in names) foreach (AssetLibrary lib in app.AssetLibraries) foreach (Asset a in lib.AppearanceAssets) if (a.DisplayName == n) return a;
        return null;
    }
    static void Look(PartDocument doc, string looks, params string[] features)
    {
        var src = Find(looks.Split('|')); if (src == null) return;
        Asset local = null;
        foreach (Asset d in doc.AppearanceAssets) if (d.DisplayName == src.DisplayName) local = d;
        if (local == null) local = src.CopyTo(doc, false);
        if (features.Length == 0) { doc.ActiveAppearance = local; return; }
        foreach (string f in features) cd.Features[f].Appearance = local;
    }

    static void Finish(PartDocument doc, string path, string desc)
    {
        doc.PropertySets["Design Tracking Properties"]["Description"].Value = desc;
        doc.SaveAs(path, false);
        var rb = cd.RangeBox;
        log.Add(string.Format("{0}: x {1:F2}..{2:F2} y {3:F2}..{4:F2} z {5:F2}..{6:F2}", System.IO.Path.GetFileName(path),
            rb.MinPoint.X*10, rb.MaxPoint.X*10, rb.MinPoint.Y*10, rb.MaxPoint.Y*10, rb.MinPoint.Z*10, rb.MaxPoint.Z*10));
        doc.Close(true);
    }

    static void BuildBase(string path)
    {
        var doc = NewPart(path);
        var J = PartFeatureOperationEnum.kJoinOperation; var C = PartFeatureOperationEnum.kCutOperation;
        double zb = BaseBottomZ, zf = SlotFloorZ;

        Rect(zb, SplitZ - zb, OutX0, OutY0, OutX1, OutY1, J, "Shell");
        Rect(zf, SplitZ - zf, InX0, InY0, InX1, InY1, C, "Pocket");

        // Pedestal under the optical module, from the floor up to the slot ceiling (the module sits on it and on the rails).
        Rect(zf, SlotCeilZ - zf, ModX0, ModY0, ModX1, ModY1, J, "Module pedestal");

        // Pouch tunnel: rails + ceiling shelf following the pouch outline, open through the -X wall.
        Poly(zf, SlotCeilZ + Plate - zf, Outline(TipX, -1, SensorY, Clr + Rail, OutX0, true), J, "Tunnel");
        Poly(zf, SlotH, Outline(TipX, -1, SensorY, Clr, OutX0 - 2, true), C, "Pouch slot");

        // Opening in the tunnel shelf where the optical module drops in.
        Rect(SlotCeilZ, 2 * Plate, ModX0, ModY0, ModX1, ModY1, C, "Module opening");

        // Corner posts the perf board sits on (board bottom at Z 0).
        Rects(zf, -zf, new[] {
            new[]{InX0, InY0, 3.5, 4.25}, new[]{86.5, InY0, InX1, 4.25},
            new[]{InX0, 66.5, 3.5, InY1}, new[]{86.5, 66.5, InX1, InY1} }, J, "Board posts");

        // USB opening in the +Y wall for a micro-USB plug's overmould.
        Rect(UsbZ - 4.0, 8.0, UsbX - 6.0, InY1 - 0.1, UsbX + 6.0, OutY1 + 0.5, C, "USB opening");

        // Finger notches above and below the pouch entrance so the pouch can be pinched out.
        Rect(zb - 0.5, Floor + 0.5, OutX0 - 1, SensorY - 10, 8, SensorY + 10, C, "Finger notch below");
        Rect(SlotCeilZ, 6.0, OutX0 - 1, SensorY - 10, 8, SensorY + 10, C, "Finger notch above");

        Look(doc, "Black|Smooth - Black");
        Finish(doc, path, "Enclosure base: rear-entry pouch tunnel with tapered stop, light chamber with sleeved 45 deg LED bores and diffuser recesses, TCS pocket");
    }

    // Optical module: drops into the base opening, sits on the pedestal and tunnel rails at the slot ceiling,
    // held down by the perf board. Holds the pad window, light chamber, aperture, TCS pocket and LED tunnels.
    static void BuildModule(string path)
    {
        var doc = NewPart(path);
        var J = PartFeatureOperationEnum.kJoinOperation; var C = PartFeatureOperationEnum.kCutOperation;
        double x0 = PocketX0 - Wall, x1 = PocketX1 + Wall, y0 = PocketY0 - Wall, y1 = PocketY1 + Wall;
        double topZ = -PocketTopGap;

        Rect(SlotCeilZ, topZ - SlotCeilZ, x0, y0, x1, y1, J, "Body");
        double cz0 = SlotCeilZ + Plate;
        double ix0 = SensorX - ChamberHalfX, ix1 = SensorX + ChamberHalfX, iy0 = SensorY - ChamberHalfY, iy1 = SensorY + ChamberHalfY;
        Rect(cz0, (PlateTopZ - Plate) - cz0, ix0, iy0, ix1, iy1, C, "Light chamber");
        Rect(SlotCeilZ, Plate, SensorX - WindowW / 2, SensorY - WindowW / 2, SensorX + WindowW / 2, SensorY + WindowW / 2, C, "Pad window");
        Circles(PlateTopZ - Plate, Plate, new[] { new[] { SensorX, SensorY } }, ApertureD / 2, C, "Sensor aperture");
        Rect(PlateTopZ, topZ - PlateTopZ, PocketX0, PocketY0, PocketX1, PocketY1, C, "TCS pocket");

        // LEDs at 45 deg in the two side walls, aimed at the pad centre: sleeve outside, bore through, diffuser recess inside.
        foreach (int side in new[] { -1, 1 })
        {
            string tag = side < 0 ? " -Y" : " +Y";
            double[] u = { 0, side * Math.Sqrt(0.5), Math.Sqrt(0.5) };                      // outward and up
            Func<double, double[]> at = d => new[] { SensorX, SensorY + side * d, PadZ + d };   // d = horizontal offset
            double sOut = side < 0 ? SensorY - y0 : y1 - SensorY;                             // outer face, from the sensor axis
            double sIn = ChamberHalfY;
            double sEnd = sOut + SleeveLen * Math.Sqrt(0.5);
            CylinderAlong(at((sOut + sEnd) / 2), u, SleeveD, SleeveLen, J, "LED sleeve" + tag);
            double b0 = sIn - 1.0, b1 = sEnd + 1.0;
            CylinderAlong(at((b0 + b1) / 2), u, LedD, (b1 - b0) * Math.Sqrt(2), C, "LED bore" + tag);
            CylinderAlong(new[] { SensorX, SensorY + side * sIn, PadZ + sIn }, new double[] { 0, side, 0 }, DiffuserD, 2 * DiffuserDepth, C, "Diffuser recess" + tag);
        }

        Look(doc, "Black|Smooth - Black");
        Finish(doc, path, "Optical module: pad window, light chamber, sensor aperture, TCS3448 pocket, sleeved 45 deg LED tunnels with diffuser recesses");
    }

    static void BuildLid(string path)
    {
        var doc = NewPart(path);
        var J = PartFeatureOperationEnum.kJoinOperation; var C = PartFeatureOperationEnum.kCutOperation;
        double top = LcdTopZ, plateBot = top - Wall;

        Rect(SplitZ, top - SplitZ, OutX0, OutY0, OutX1, OutY1, J, "Shell");
        Rect(SplitZ, plateBot - SplitZ, InX0, InY0, InX1, InY1, C, "Pocket");
        double lx0 = InX0 + 0.2, ly0 = InY0 + 0.2, lx1 = InX1 - 0.2, ly1 = InY1 - 0.2;
        Rect(2.0, 4.0, lx0, ly0, lx1, ly1, J, "Lip");
        Rect(SplitZ, 2.0, InX0, InY0, InX1, InY1, J, "Lip bridge");
        Rect(2.0, 4.0 + SplitZ, lx0 + 1.2, ly0 + 1.2, lx1 - 1.2, ly1 - 1.2, C, "Lip inside");
        Rect(plateBot, Wall, BezelX0 - 0.3, BezelY0 - 0.3, BezelX1 + 0.3, BezelY1 + 0.3, C, "LCD window");
        Circles(LcdPcbTopZ, plateBot - LcdPcbTopZ, LcdHoles, 3.0, J, "LCD bosses");
        Circles(LcdPcbTopZ, 4.0, LcdHoles, 1.25, C, "LCD screw holes");
        Rects(1.7, plateBot - 1.7, new[] {
            new[]{0.0, 0.0, 3.5, 3.5}, new[]{86.5, 0.0, 90.0, 3.5},
            new[]{0.0, 66.5, 3.5, 70.0}, new[]{86.5, 66.5, 90.0, 70.0} }, J, "Board clamps");

        Look(doc, "Black|Smooth - Black");
        Finish(doc, path, "Enclosure lid: LCD window (LCD rotated 90 deg) and bosses, board clamps, locating lip");
    }

    static void BuildPouch(string path)
    {
        var doc = NewPart(path);
        // Local frame: tip at x = 0, body toward +x, centred on y = 0, back face z = 0, front (pad) face z = PouchT.
        Poly(0, PouchT, Outline(0, 1, 0, 0, double.NaN), PartFeatureOperationEnum.kJoinOperation, "Body");
        Rect(PouchT, 0.2, PadS0, -PadHalf, PadS1, PadHalf, PartFeatureOperationEnum.kJoinOperation, "Pad");
        Look(doc, "Black|Smooth - Black", "Body");
        Look(doc, "White|Smooth - White|Plastic - Glossy (White)", "Pad");
        Finish(doc, path, "DetectaChem MobileDetect pouch, outline from product image at 50.3 x 89.9 mm; thickness assumed");
    }

    public static string Build(object appObj, string root)
    {
        app = (Application)appObj; tg = app.TransientGeometry; log = new List<string>();
        string encl = System.IO.Path.Combine(root, "Enclosure");
        string pouchDir = System.IO.Path.Combine(root, "Purchased Parts", "DetectaChem");
        System.IO.Directory.CreateDirectory(encl); System.IO.Directory.CreateDirectory(pouchDir);
        string basePath = System.IO.Path.Combine(encl, "Enclosure Base.ipt");
        string lidPath = System.IO.Path.Combine(encl, "Enclosure Lid.ipt");
        string modulePath = System.IO.Path.Combine(encl, "Optical Module.ipt");
        string pouchPath = System.IO.Path.Combine(pouchDir, "MobileDetect Pouch (approx).ipt");
        string asmPath = System.IO.Path.Combine(root, "Enclosure Assembly.iam");
        if (System.IO.File.Exists(asmPath)) throw new Exception("Refusing to overwrite " + asmPath);

        BuildBase(basePath);
        BuildModule(modulePath);
        BuildLid(lidPath);
        if (!System.IO.File.Exists(pouchPath)) BuildPouch(pouchPath);

        string atpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kAssemblyDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var asm = (AssemblyDocument)app.Documents.Add(DocumentTypeEnum.kAssemblyDocumentObject, atpl, true);
        var occs = asm.ComponentDefinition.Occurrences;
        var id = tg.CreateMatrix();
        foreach (var p in new[] { System.IO.Path.Combine(root, "Electronics Assembly.iam"), basePath, modulePath, lidPath })
            occs.Add(p, id).Grounded = true;
        // Pouch: tip toward +X at TipX, body running back out through the -X wall.
        var m = tg.CreateMatrix();
        m.SetCoordinateSystem(P3(TipX, SensorY, SlotFloorZ), tg.CreateVector(-1, 0, 0), tg.CreateVector(0, -1, 0), tg.CreateVector(0, 0, 1));
        occs.Add(pouchPath, m).Grounded = true;
        asm.PropertySets["Design Tracking Properties"]["Description"].Value = "Reader enclosure with electronics and a pouch inserted";
        asm.SaveAs(asmPath, false);

        log.Add(string.Format("Pad surface Z {0:F2}; slot floor {1:F2}; slot ceiling {2:F2}; chamber plate top {3:F2}; pouch tip X {4:F2}; pouch sticks out {5:F2}",
            PadZ, SlotFloorZ, SlotCeilZ, PlateTopZ, TipX, OutX0 - (TipX - 89.8)));
        log.Add(string.Format("Outside: {0:F1} x {1:F1} x {2:F1} mm", OutX1 - OutX0, OutY1 - OutY0, LcdTopZ - BaseBottomZ));
        return string.Join("\n", log);
    }
}
