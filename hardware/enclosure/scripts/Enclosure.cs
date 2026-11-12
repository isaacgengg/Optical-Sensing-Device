// Generates the enclosure parts in Inventor 2027 for the dual-sensor layout: Enclosure Base, Optical Module,
// Enclosure Lid, the MobileDetect pouch (only if missing), and Enclosure Assembly (wrapping Electronics Assembly).
// Run with build_enclosure.ps1.
//
// All numbers are millimetres in the Electronics Assembly frame: perf board lower-left corner, +Z up, perf board
// bottom at Z = 0. The pouch enters through the -X wall (under the ESP32), tip first, and stops in a tapered pocket
// with its pad under the TCS3448. The LFA strip enters through the +Y wall in a groove in the tunnel floor (flush,
// so the pouch slides over it) with its lines under the two sensors. Each sensor looks through a small aperture
// aimed at its LFA line; the onboard white LED of each breakout lights its zone through an opening in the plate.
using System;
using System.Collections.Generic;
using Inventor;

public static class Enclosure
{
    // ---------- Fixed by the electronics assembly ----------
    const double SensorX = 70.40, TcsY = 34.62, AsY = 16.84, SensorFaceZ = -4.75;
    const double TcsLedY = 41.99, AsLedY = 24.21, LedZ = -4.35;      // onboard white LEDs, +Y of each sensor (X = SensorX)
    const double BoardsX0 = 57.70, BoardsX1 = 83.10, BoardsY0 = 8.33, BoardsY1 = 43.89, BoardsJoinY = 26.11;
    const double BoardsLowestZ = -6.55;                               // STEMMA QT connectors (X outside 63.1..77.7)
    const double TcsPinsY0 = 27.38, TcsPinsY1 = 29.92, AsPinsY0 = 9.60, AsPinsY1 = 12.14;   // header pin ends, Z -5.5
    const double EspLowestZ = -7.20;
    const double UsbX = 23.41, UsbZ = -5.40;                          // ESP32 micro-USB centre, facing +Y
    const double LcdPcbTopZ = 22.1, LcdTopZ = 29.3;
    const double BezelX0 = 32.65, BezelX1 = 56.85, BezelY0 = -0.6, BezelY1 = 70.6;
    static readonly double[][] LcdHoles = { new[]{29.25,72.5}, new[]{29.25,-2.5}, new[]{60.25,72.5}, new[]{60.25,-2.5} };

    // ---------- LFA strip (measured) ----------
    const double TLineY = 30.73, CLineY = 20.73;                      // lines centred between the sensors
    const double StripStopY = 10.73;                                  // testing end, fully inserted
    const double StripW = 3.175, StripT = 0.5;                        // StripT ASSUMED
    const double StripTopZ = -18.0;                                   // 0.1 mm below the tunnel floor
    const double GrooveClr = 0.2;

    // ---------- Pouch (measured: 3.75 x 2 in, 1 in front diagonals, 0.5 in second diagonals, 0.75 in pad) ----------
    // Half-width at distance s from the bottom tip. Diagonal angles (30.5 and 15.9 deg) are from the product image.
    static readonly double[][] PouchHalf = {
        new[]{0,9.03}, new[]{21.89,21.92}, new[]{34.10,25.40}, new[]{81.90,25.40},
        new[]{86.30,23.80}, new[]{90.90,19.50}, new[]{95.25,15.15} };
    const double PouchLen = 95.25;
    const double PadS0 = 2.62, PadS1 = 21.67, PadHalf = 9.525, PadCentreS = 12.15;
    const double FilmT = 0.8, PadT = 2.0;                             // measured
    const double BulgeS0 = 42, BulgeS1 = 68, BulgeHalf = 18, BulgeT = 7.9;   // ampoules: position ASSUMED, height = spec max
    const double Clr = 0.5;
    const double Rail = 2.0;

    // ---------- Heights ----------
    const double SlotFloorZ = -17.9;     // tunnel floor (= strip top, flush)
    const double TallCeilZ = -9.0;       // tunnel ceiling near the entrance (pouch's thick part passes under the ESP32)
    const double ShelfTopZ = -7.8;       // 0.6 mm under the ESP32
    const double LowH = 3.0;             // tunnel height under the sensors (pad end is 2 mm max)
    const double StepX = 44.0;           // tall -> low; the ampoules stop at X < 40.5 when fully inserted
    const double Plate = 1.5;            // aperture plate = module floor

    // ---------- Optics ----------
    // Each sensor sits over a solid black block with two angled bores: a view bore from the sensor down to an
    // aperture aimed at its LFA line, and an LED bore from the onboard LED to an exit slot just beside the aperture.
    const double ApertureD = 1.6;        // view spot on the strip ~2.6 mm along it, within the 3.175 mm width
    const double ViewBoreExtra = 0.4;    // view bore above the aperture is ApertureD + this wide
    const double LedBoreD = 1.8;
    const double SlotLen = 2.2, SlotHalfX = 1.5, Web = 0.4;   // LED exit slot beside the aperture, Web from its rim
    const double BlockX0 = 63.5, BlockX1 = 77.3;               // clear of the STEMMA QT connectors
    const double BlockTopZ = -5.0, PinRecessZ = -5.9;          // 0.25 mm under the board parts; deeper under header pins
    // Read chamber recessed into the module's underside above the read zone, so the LED light has room to land
    // where each sensor looks (the pouch pad is only 1 mm below the module). Split between the sensors by a wall.
    const double ChamberTopZ = -12.0, ChamberX0 = 66.4, ChamberX1 = 74.4, ChamberY0 = 16.0, ChamberY1 = 37.0;
    const double ChamberWallT = 1.0;
    const double AperturePlate = 1.5;                          // apertures and LED slots go through the chamber's ceiling

    // ---------- Shell ----------
    const double Wall = 2.0, Floor = 2.0;
    const double InX0 = -1.5, InX1 = 91.0, InY0 = -6.0, InY1 = 79.5;
    const double SplitZ = 4.0;
    const double PocketClr = 0.5, PocketTopGap = 0.2, ModFit = 0.2;

    // Derived
    static double LowCeilZ { get { return SlotFloorZ + LowH; } }                  // module bottom
    static double PlateTopZ { get { return LowCeilZ + Plate; } }
    static double PlateMidZ { get { return LowCeilZ + Plate / 2; } }
    static double PadTopZ { get { return SlotFloorZ + PadT; } }
    static double StripDepth { get { return SensorFaceZ - StripTopZ; } }
    static double PlateDepth { get { return SensorFaceZ - PlateMidZ; } }
    static double PadDepth { get { return SensorFaceZ - PadTopZ; } }
    static double ApertureTY { get { return TcsY - (TcsY - TLineY) * PlateDepth / StripDepth; } }
    static double ApertureCY { get { return AsY + (CLineY - AsY) * PlateDepth / StripDepth; } }
    static double PouchY { get { return TcsY - (TcsY - ApertureTY) * PadDepth / PlateDepth; } }   // TCS spot on the pad
    static double TipX { get { return SensorX + PadCentreS; } }
    static double OutX0 { get { return InX0 - Wall; } }
    static double OutX1 { get { return InX1 + Wall; } }
    static double OutY0 { get { return InY0 - Wall; } }
    static double OutY1 { get { return InY1 + Wall; } }
    static double BaseBottomZ { get { return SlotFloorZ - Floor; } }
    static double PocketX0 { get { return BoardsX0 - PocketClr; } }
    static double PocketX1 { get { return BoardsX1 + PocketClr; } }
    static double PocketY0 { get { return BoardsY0 - PocketClr; } }
    static double PocketY1 { get { return BoardsY1 + PocketClr; } }
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

    // Pouch outline offset outward by c, tip at tipX, body running in direction dir (+1 = +X, -1 = -X),
    // centred on y = cy, cut off at x = xEnd (NaN = whole pouch). slideOut = true never narrows away from the
    // tip, so the whole pouch can slide out through it.
    static List<double[]> Outline(double tipX, double dir, double cy, double c, double xEnd, bool slideOut = false)
    {
        var side = new List<double[]>();
        double widest = 0, endHalf = double.NaN;
        for (int i = 0; i < PouchHalf.Length; i++)
        {
            var p = PouchHalf[i];
            double x = tipX + dir * p[0];
            if (!double.IsNaN(xEnd) && dir * (x - xEnd) > 0)
            {
                // Cut off between this point and the previous one: interpolate the pouch's half-width at xEnd.
                var q = PouchHalf[i - 1];
                double sEnd = dir * (xEnd - tipX);
                endHalf = q[1] + (p[1] - q[1]) * (sEnd - q[0]) / (p[0] - q[0]);
                break;
            }
            widest = Math.Max(widest, p[1]);
            side.Add(new[] { x, cy - ((slideOut ? widest : p[1]) + c) });
        }
        side[0] = new[] { tipX - dir * c, cy - (PouchHalf[0][1] + c) };
        if (!double.IsNaN(xEnd))
        {
            double h = double.IsNaN(endHalf) ? cy - side[side.Count - 1][1] - c : endHalf;
            if (slideOut) h = Math.Max(h, widest);
            side.Add(new[] { xEnd, cy - (h + c) });
        }
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

        // Pedestal under the optical module (outside the tunnel), floor up to the module's underside.
        Rect(zf, LowCeilZ - zf, ModX0, ModY0, ModX1, ModY1, J, "Module pedestal");

        // Pouch tunnel: rails + ceiling shelf following the pouch outline, open through the -X wall. Tall near the
        // entrance for the pouch's thick part, low (3 mm) from StepX on, where only the thin pad end passes.
        Poly(zf, ShelfTopZ - zf, Outline(TipX, -1, PouchY, Clr + Rail, OutX0, true), J, "Tunnel");
        Poly(zf, TallCeilZ - zf, Outline(TipX, -1, PouchY, Clr, OutX0 - 2, true), C, "Pouch slot");
        double half = 25.40 + Clr + Rail;
        Rect(LowCeilZ, TallCeilZ - LowCeilZ, StepX, PouchY - half, ModX0, PouchY + half, J, "Low ceiling");

        // Opening where the optical module drops in; its floor (the aperture plate) is the tunnel ceiling there.
        Rect(LowCeilZ, SplitZ - LowCeilZ, ModX0, ModY0, ModX1, ModY1, C, "Module opening");

        // LFA strip: groove in the floor (strip top flush with it) from the stop out through the +Y wall.
        double gx0 = SensorX - StripW / 2 - GrooveClr, gx1 = SensorX + StripW / 2 + GrooveClr, gd = StripT + GrooveClr;
        Rect(zf - gd, gd, gx0, StripStopY - 0.3, gx1, OutY1 + 1, C, "Strip groove");
        Rect(zf, 0.6, gx0, InY1 - 0.1, gx1, OutY1 + 1, C, "Strip slot");

        // Corner posts the perf board sits on (board bottom at Z 0).
        Rects(zf, -zf, new[] {
            new[]{InX0, InY0, 3.5, 4.25}, new[]{86.5, InY0, InX1, 4.25},
            new[]{InX0, 66.5, 3.5, InY1}, new[]{86.5, 66.5, InX1, InY1} }, J, "Board posts");

        // USB opening in the +Y wall for a micro-USB plug's overmould.
        Rect(UsbZ - 4.0, 8.0, UsbX - 6.0, InY1 - 0.1, UsbX + 6.0, OutY1 + 0.5, C, "USB opening");

        // Finger notches above and below the pouch entrance.
        Rect(zb - 0.5, Floor + 0.5, OutX0 - 1, PouchY - 10, 8, PouchY + 10, C, "Finger notch below");
        Rect(TallCeilZ, 6.0, OutX0 - 1, PouchY - 10, 8, PouchY + 10, C, "Finger notch above");

        Look(doc, "Black|Smooth - Black");
        Finish(doc, path, "Enclosure base: rear-entry pouch tunnel (tall at the entrance, 3 mm under the sensors), LFA strip groove, module opening");
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

    // Cylindrical cut along the line from a to b (both in mm), exactly from a to b.
    static void BoreBetween(double[] a, double[] b, double d, string name)
    {
        double[] u = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
        double len = Math.Sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        CylinderAlong(new[] { (a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2 }, u, d, len, PartFeatureOperationEnum.kCutOperation, name);
    }

    // Point on a sensor's view axis (sensor centre -> its LFA line on the strip) at height z.
    static double ViewY(double sensorY, double lineY, double z) { return sensorY + (lineY - sensorY) * (SensorFaceZ - z) / (SensorFaceZ - StripTopZ); }

    // Optical module: drops into the base opening and sits on the pedestal and rails; the perf board holds it down.
    // A pocket holds both breakouts. Under them a solid black block carries, per sensor, a view bore (sensor ->
    // aperture in the floor plate, aimed at its LFA line) and an LED bore (onboard LED -> exit slot just +Y of the
    // aperture). Nothing else is open, so the sensors only see light coming back up their own view bore.
    static void BuildModule(string path, double apD)
    {
        var doc = NewPart(path);
        var J = PartFeatureOperationEnum.kJoinOperation; var C = PartFeatureOperationEnum.kCutOperation;
        double x0 = PocketX0 - Wall, x1 = PocketX1 + Wall, y0 = PocketY0 - Wall, y1 = PocketY1 + Wall;
        double topZ = -PocketTopGap;

        Rect(LowCeilZ, topZ - LowCeilZ, x0, y0, x1, y1, J, "Body");
        Rect(PlateTopZ, topZ - PlateTopZ, PocketX0, PocketY0, PocketX1, PocketY1, C, "Sensor pocket");
        Rect(PlateTopZ, BlockTopZ - PlateTopZ, BlockX0, BoardsY0 + 0.3, BlockX1, BoardsY1 - 0.3, J, "Optical block");
        Rects(PinRecessZ, BlockTopZ - PinRecessZ + 0.1, new[] {
            new[]{ BlockX0 - 0.5, TcsPinsY0 - 0.3, BlockX1 + 0.5, TcsPinsY1 + 0.3 },
            new[]{ BlockX0 - 0.5, AsPinsY0 - 0.3, BlockX1 + 0.5, AsPinsY1 + 0.3 } }, C, "Header pin recesses");

        // Read chamber in the underside, split by a wall where the two boards meet.
        Rects(LowCeilZ, ChamberTopZ - LowCeilZ, new[] {
            new[]{ ChamberX0, ChamberY0, ChamberX1, BoardsJoinY - ChamberWallT / 2 },
            new[]{ ChamberX0, BoardsJoinY + ChamberWallT / 2, ChamberX1, ChamberY1 } }, C, "Read chamber");

        var info = new List<string>();
        double apZ0 = ChamberTopZ, apZ1 = ChamberTopZ + AperturePlate, apZm = (apZ0 + apZ1) / 2;
        foreach (var s in new[] { new { Tag = "TCS3448", Y = TcsY, Line = TLineY, Led = TcsLedY },
                                  new { Tag = "AS7343",  Y = AsY,  Line = CLineY, Led = AsLedY } })
        {
            double[] u = { 0, s.Line - s.Y, StripTopZ - SensorFaceZ };
            double axial = Math.Sqrt(u[1] * u[1] + u[2] * u[2]) / Math.Abs(u[2]);      // axial length per mm of height
            double cos = 1 / axial;
            // Aperture through the chamber ceiling and the wider view bore above it, up through the block top.
            CylinderAlong(new[] { SensorX, ViewY(s.Y, s.Line, apZm), apZm }, u, apD, (AperturePlate + 0.2) * axial, C, "Aperture " + s.Tag);
            double zv0 = apZ1, zv1 = BlockTopZ + 0.5, zvm = (zv0 + zv1) / 2;
            CylinderAlong(new[] { SensorX, ViewY(s.Y, s.Line, zvm), zvm }, u, apD + ViewBoreExtra, (zv1 - zv0) * axial, C, "View bore " + s.Tag);

            // LED exit slot through the chamber ceiling, just +Y of the aperture, and the LED bore from the LED down to it.
            double apMaxY = Math.Max(ViewY(s.Y, s.Line, apZ1), ViewY(s.Y, s.Line, apZ0)) + apD / 2 / cos;
            double slot0 = apMaxY + Web, slot1 = slot0 + SlotLen, slotC = (slot0 + slot1) / 2;
            Rect(apZ0, AperturePlate, SensorX - SlotHalfX, slot0, SensorX + SlotHalfX, slot1, C, "LED slot " + s.Tag);
            BoreBetween(new[] { SensorX, s.Led, LedZ }, new[] { SensorX, slotC, apZ1 }, LedBoreD, "LED bore " + s.Tag);

            // Thinnest wall between the LED bore and the view bore over the block's height.
            double minWall = double.MaxValue;
            for (double z = apZ1; z <= BlockTopZ; z += 0.1)
            {
                double yl = s.Led + (slotC - s.Led) * (LedZ - z) / (LedZ - apZ1);
                double yv = ViewY(s.Y, s.Line, z);
                minWall = Math.Min(minWall, Math.Abs(yl - yv) * cos - (apD + ViewBoreExtra) / 2 - LedBoreD / 2);
            }
            // Where the LED's light lands on the pouch pad and the strip, against where the sensor looks.
            Func<double, double, double, double> land = (ledY, exitY, zt) => exitY + (exitY - ledY) * (apZ0 - zt) / (LedZ - apZ0);
            double padLo = land(s.Led + 1.4, slot0, PadTopZ), padHi = land(s.Led - 1.4, slot1, PadTopZ);
            double stripLo = land(s.Led + 1.4, slot0, StripTopZ), stripHi = land(s.Led - 1.4, slot1, StripTopZ);
            info.Add(string.Format("{0}: aperture Y {1:F2}, LED slot Y {2:F2}..{3:F2}, thinnest bore wall {4:F2} mm; " +
                "pad: views {5:F2}, lit {6:F2}..{7:F2}; strip: views {8:F2} (line {9:F2}), lit {10:F2}..{11:F2}",
                s.Tag, ViewY(s.Y, s.Line, apZm), slot0, slot1, minWall,
                ViewY(s.Y, s.Line, PadTopZ), Math.Min(padLo, padHi), Math.Max(padLo, padHi),
                ViewY(s.Y, s.Line, StripTopZ), s.Line, Math.Min(stripLo, stripHi), Math.Max(stripLo, stripHi)));
        }

        Look(doc, "Black|Smooth - Black");
        Finish(doc, path, string.Format("Optical module: pocket for both breakouts, solid optical block with an aimed view bore (aperture {0} mm) and an LED bore per sensor", apD));
        foreach (var i in info) log.Add("  " + i);
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
        var J = PartFeatureOperationEnum.kJoinOperation;
        // Local frame: tip at x = 0, body toward +x, centred on y = 0, back face z = 0.
        Poly(0, FilmT, Outline(0, 1, 0, 0, double.NaN), J, "Body");
        Rect(FilmT, PadT - FilmT, PadS0, -PadHalf, PadS1, PadHalf, J, "Pad");
        Rect(FilmT, BulgeT - FilmT, BulgeS0, -BulgeHalf, BulgeS1, BulgeHalf, J, "Ampoules");
        Look(doc, "Black|Smooth - Black", "Body", "Ampoules");
        Look(doc, "White|Smooth - White|Plastic - Glossy (White)", "Pad");
        Finish(doc, path, "DetectaChem MobileDetect pouch, measured 95.25 x 50.8 mm, film 0.8 mm, pad 19.05 mm square x 2 mm; ampoule bulge position assumed");
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
        string pouchPath = System.IO.Path.Combine(pouchDir, "MobileDetect Pouch.ipt");
        string asmPath = System.IO.Path.Combine(root, "Enclosure Assembly.iam");
        if (System.IO.File.Exists(asmPath)) throw new Exception("Refusing to overwrite " + asmPath);

        BuildBase(basePath);
        BuildModule(modulePath, ApertureD);
        BuildLid(lidPath);
        if (!System.IO.File.Exists(pouchPath)) BuildPouch(pouchPath);

        string atpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kAssemblyDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var asm = (AssemblyDocument)app.Documents.Add(DocumentTypeEnum.kAssemblyDocumentObject, atpl, true);
        var occs = asm.ComponentDefinition.Occurrences;
        var id = tg.CreateMatrix();
        foreach (var p in new[] { System.IO.Path.Combine(root, "Electronics Assembly.iam"), basePath, modulePath, lidPath })
            occs.Add(p, id).Grounded = true;
        AddPouch(asm, root);
        asm.PropertySets["Design Tracking Properties"]["Description"].Value = "Reader enclosure with electronics, a pouch and an LFA strip inserted";
        asm.SaveAs(asmPath, false);

        log.Add(string.Format("Floor (strip top) Z {0:F2}: {1:F2} mm below the sensors; pad top Z {2:F2}: {3:F2} mm; module underside Z {4:F2}",
            SlotFloorZ, StripDepth, PadTopZ, PadDepth, LowCeilZ));
        log.Add(string.Format("Pouch centreline Y {0:F2}, tip X {1:F2}, sticks out {2:F2} mm", PouchY, TipX, OutX0 - (TipX - PouchLen)));
        log.Add(string.Format("Outside: {0:F1} x {1:F1} x {2:F1} mm", OutX1 - OutX0, OutY1 - OutY0, LcdTopZ - BaseBottomZ));
        return string.Join("\n", log);
    }

    // Adds the pouch, fully inserted and grounded in place (PouchSlide.cs then makes it slide): tip toward +X at
    // TipX, body running back out toward -X. Builds the pouch part first if it is missing.
    static void AddPouch(AssemblyDocument asm, string root)
    {
        string pouchPath = System.IO.Path.Combine(root, "Purchased Parts", "DetectaChem", "MobileDetect Pouch.ipt");
        if (!System.IO.File.Exists(pouchPath)) BuildPouch(pouchPath);
        var m = tg.CreateMatrix();
        m.SetCoordinateSystem(P3(TipX, PouchY, SlotFloorZ), tg.CreateVector(-1, 0, 0), tg.CreateVector(0, -1, 0), tg.CreateVector(0, 0, 1));
        asm.ComponentDefinition.Occurrences.Add(pouchPath, m).Grounded = true;
    }

    // ---------- Optics test jig ----------
    // A 2 mm floor plate under the whole perf board with posts at its corners, and a block under the optical module
    // with the module's locating pocket, the pouch slot (3 mm, open toward -X) and the strip groove (open toward +Y),
    // all at the enclosure's heights. The perf board rests on the posts and the module; cover it with a dark box
    // when measuring.
    static void BuildSled(string path)
    {
        var doc = NewPart(path);
        var J = PartFeatureOperationEnum.kJoinOperation; var C = PartFeatureOperationEnum.kCutOperation;
        double zb = BaseBottomZ, zf = SlotFloorZ;
        double rim = 2.0;
        double sx0 = ModX0 - rim, sx1 = ModX1 + rim, sy0 = ModY0 - rim, sy1 = ModY1 + rim;

        Rect(zb, Floor, 0, 0, 90, 70, J, "Floor plate");
        Rect(zb, (LowCeilZ + rim) - zb, sx0, sy0, sx1, sy1, J, "Module block");
        Rect(LowCeilZ, rim + 0.1, ModX0, ModY0, ModX1, ModY1, C, "Module pocket");
        Poly(zf, LowH, Outline(TipX, -1, PouchY, Clr, sx0 - 2, true), C, "Pouch slot");
        double gx0 = SensorX - StripW / 2 - GrooveClr, gx1 = SensorX + StripW / 2 + GrooveClr, gd = StripT + GrooveClr;
        Rect(zf - gd, gd, gx0, StripStopY - 0.3, gx1, 71, C, "Strip groove");
        Rects(zf, -zf, new[] { new[]{0.0, 0.0, 4.0, 4.0}, new[]{86.0, 0.0, 90.0, 4.0}, new[]{0.0, 66.0, 4.0, 70.0}, new[]{86.0, 66.0, 90.0, 70.0} }, J, "Board posts");

        Look(doc, "Black|Smooth - Black");
        Finish(doc, path, "Optics test jig: floor plate, board posts, module pocket, pouch slot and strip groove at the enclosure's heights");
    }

    public static string BuildJig(object appObj, string root, double[] apertures)
    {
        app = (Application)appObj; tg = app.TransientGeometry; log = new List<string>();
        string dir = System.IO.Path.Combine(root, "Test Jig");
        System.IO.Directory.CreateDirectory(dir);
        string sledPath = System.IO.Path.Combine(dir, "Test Sled.ipt");
        string asmPath = System.IO.Path.Combine(root, "Optics Test Jig.iam");
        if (System.IO.File.Exists(asmPath)) throw new Exception("Refusing to overwrite " + asmPath);
        BuildSled(sledPath);
        string first = null;
        foreach (double d in apertures)
        {
            string p = System.IO.Path.Combine(dir, string.Format(System.Globalization.CultureInfo.InvariantCulture, "Optical Module d{0:0.0}.ipt", d));
            BuildModule(p, d);
            if (first == null || Math.Abs(d - ApertureD) < 1e-6) first = p;
        }
        string atpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kAssemblyDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var asm = (AssemblyDocument)app.Documents.Add(DocumentTypeEnum.kAssemblyDocumentObject, atpl, true);
        var id = tg.CreateMatrix();
        foreach (var p in new[] { System.IO.Path.Combine(root, "Electronics Assembly.iam"), sledPath, first })
            asm.ComponentDefinition.Occurrences.Add(p, id).Grounded = true;
        AddPouch(asm, root);
        asm.PropertySets["Design Tracking Properties"]["Description"].Value = "Optics test jig: electronics on the sled with one optical module, a pouch and an LFA strip";
        asm.SaveAs(asmPath, false);
        return string.Join("\n", log);
    }

    // For the other scripts.
    public static double PouchTipX { get { return TipX; } }
    public static double PouchCentreY { get { return PouchY; } }
    public static double FloorZ { get { return SlotFloorZ; } }
}
