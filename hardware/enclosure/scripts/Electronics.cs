// Places the boards in Electronics Assembly.iam for the dual-sensor layout (PR #19). Run with build_electronics.ps1.
//
// Coordinates are millimetres in the assembly frame: perf board lower-left corner, +Z up, board bottom Z = 0.
// Perf grid: HoleX(i) = 8.17 + 2.54 i (i = 0..29), HoleY(j) = 5.79 + 2.54 j (j = 0..23).
// Both sensor breakouts hang below the board, component side down, on 1x6 headers with a 2.5 mm spacer.
using System;
using System.Collections.Generic;
using Inventor;

public static class Electronics
{
    const double P = 2.54, X0 = 8.17, Y0 = 5.79;
    static double HX(int i) { return X0 + P * i; }
    static double HY(int j) { return Y0 + P * j; }

    // ESP32 (matches the real board): pins in columns 1 and 11, nearest-USB pin pair in row 21
    // (rows 22-23 free above it), USB toward +Y.
    const int EspColLeft = 1, EspRowUsbPin = 21;
    // TCS3448 header along a row (board turned 90 deg, extending toward -Y from its header), pins in columns 22-27.
    // AS7343 the same way round, 7 rows further toward -Y: the boards' edges touch and the sensors are 17.78 mm
    // apart, the closest they can get. The LFA lines are 10 mm apart, so each sits 3.89 mm inward of its sensor and
    // the optical module's slit windows aim each sensor at its line.
    const int SensorCol0 = 22, TcsHeaderRow = 14, AsHeaderRow = 7;
    const double SpacerZ = -2.5;    // breakout back face, below the header's plastic spacer
    // Perf board corner mounting holes (approximate, from the photo of the real board).
    const double MountHoleD = 2.0, MountHoleInset = 2.5;

    static Application app; static TransientGeometry tg;

    static Matrix Mat(double ox, double oy, double oz, double[] xa, double[] ya, double[] za)
    {
        var m = tg.CreateMatrix();
        m.SetCoordinateSystem(tg.CreatePoint(ox / 10, oy / 10, oz / 10), tg.CreateVector(xa[0], xa[1], xa[2]),
                              tg.CreateVector(ya[0], ya[1], ya[2]), tg.CreateVector(za[0], za[1], za[2]));
        return m;
    }

    static void Place(ComponentOccurrence o, Matrix m) { o.Grounded = false; o.Transformation = m; o.Grounded = true; }

    static string Box(ComponentOccurrence o)
    {
        var rb = o.RangeBox;
        return string.Format("{0,-44} x {1,6:F2}..{2,6:F2}  y {3,6:F2}..{4,6:F2}  z {5,6:F2}..{6,6:F2}", o.Name,
            rb.MinPoint.X*10, rb.MaxPoint.X*10, rb.MinPoint.Y*10, rb.MaxPoint.Y*10, rb.MinPoint.Z*10, rb.MaxPoint.Z*10);
    }

    static void AddMountingHoles(string perfPath, List<string> log)
    {
        var doc = (PartDocument)app.Documents.Open(perfPath, false);
        var cd = doc.ComponentDefinition;
        foreach (PartFeature f in cd.Features) if (f.Name == "Mounting holes") { log.Add("perf board already has mounting holes"); doc.Close(true); return; }
        var sk = cd.Sketches.Add(cd.WorkPlanes[3]);
        double a = MountHoleInset, bx = 90 - MountHoleInset, by = 70 - MountHoleInset;
        foreach (var c in new[] { new[] { a, a }, new[] { bx, a }, new[] { a, by }, new[] { bx, by } })
            sk.SketchCircles.AddByCenterRadius(tg.CreatePoint2d(c[0] / 10, c[1] / 10), MountHoleD / 20);
        var def = cd.Features.ExtrudeFeatures.CreateExtrudeDefinition(sk.Profiles.AddForSolid(), PartFeatureOperationEnum.kCutOperation);
        def.SetThroughAllExtent(PartFeatureExtentDirectionEnum.kPositiveExtentDirection);
        var feat = cd.Features.ExtrudeFeatures.Add(def); feat.Name = "Mounting holes"; sk.Visible = false;
        doc.Save(); doc.Close(true);
        log.Add("perf board: added 4 x " + MountHoleD + " mm corner mounting holes");
    }

    public static string Run(object appObj, string root)
    {
        app = (Application)appObj; tg = app.TransientGeometry; var log = new List<string>();
        string asmPath = System.IO.Path.Combine(root, "Electronics Assembly.iam");
        string tcsPath = System.IO.Path.Combine(root, "Purchased Parts", "Adafruit", "6525 TCS3448 Light Color Sensor", "6525 TCS3448 Light Color Sensor.iam");
        string hdrPath = System.IO.Path.Combine(root, "Electronics", "Header 1x6 (TCS3448).ipt");
        string perfPath = System.IO.Path.Combine(root, "Electronics", "Perfboard 70x90.ipt");

        AddMountingHoles(perfPath, log);

        var asm = (AssemblyDocument)app.Documents.Open(asmPath, true);
        var occs = asm.ComponentDefinition.Occurrences;
        ComponentOccurrence esp = null, tcs = null, tcsHdr = null, asOcc = null, asHdr = null;
        var toDelete = new List<ComponentOccurrence>();
        foreach (ComponentOccurrence o in occs)
        {
            if (o.Name.StartsWith("ESP32")) esp = o;
            else if (o.Name.StartsWith("6525 TCS3448")) tcs = o;
            else if (o.Name.StartsWith("Header 1x6 (TCS3448)")) tcsHdr = o;
            else if (o.Name.StartsWith("AS7343")) asOcc = o;
            else if (o.Name.StartsWith("Header 1x6 (AS7343)")) asHdr = o;
            else if (o.Name.StartsWith("Wiring")) toDelete.Add(o);
        }
        foreach (var o in toDelete) { o.Delete(); log.Add("removed old wiring occurrence"); }

        // ESP32: local x = USB edge toward antenna, local -y = right-hand column (3V3 ... D23), component side +z.
        // Here: local x -> -Y (USB toward +Y), local y -> -X, local z -> -Z (hanging below the board).
        double pin0 = 51.5 / 2 - 7 * P;
        double espTx = HX(EspColLeft) + 12.7, espTy = HY(EspRowUsbPin) + pin0;
        Place(esp, Mat(espTx, espTy, SpacerZ, new double[] { 0, -1, 0 }, new double[] { -1, 0, 0 }, new double[] { 0, 0, -1 }));

        // Breakouts: local header row at y = 2.54 with pins at x = 6.35 .. 19.05 (pin 1 = VIN at x = 6.35).
        // Turned so local x -> +X, local y -> -Y (board extends toward -Y from its header), local z -> -Z.
        Func<int, Matrix> sensorMat = row => Mat(HX(SensorCol0) - 6.35, HY(row) + 2.54, SpacerZ,
                                                 new double[] { 1, 0, 0 }, new double[] { 0, -1, 0 }, new double[] { 0, 0, -1 });
        Place(tcs, sensorMat(TcsHeaderRow));
        Place(tcsHdr, sensorMat(TcsHeaderRow));
        // The AS7343 breakout (Adafruit 6477) has the same outline, holes, sensor position and header pinout as the
        // TCS3448 breakout (compared in Adafruit's board files), so it reuses that model.
        if (asOcc == null) { asOcc = occs.Add(tcsPath, sensorMat(AsHeaderRow)); asOcc.Name = "AS7343 (same board as TCS3448)"; }
        Place(asOcc, sensorMat(AsHeaderRow));
        if (asHdr == null) { asHdr = occs.Add(hdrPath, sensorMat(AsHeaderRow)); asHdr.Name = "Header 1x6 (AS7343)"; }
        Place(asHdr, sensorMat(AsHeaderRow));
        tcs.Name = "6525 TCS3448 Light Color Sensor";

        asm.Update(); asm.Save();
        foreach (ComponentOccurrence o in occs) log.Add(Box(o));
        double sx = HX(SensorCol0) - 6.35 + 12.7;
        log.Add(string.Format("TCS3448 sensor centre ({0:F2}, {1:F2}); AS7343 sensor centre ({0:F2}, {2:F2}); face Z {3:F2}",
            sx, HY(TcsHeaderRow) + 2.54 - 8.509, HY(AsHeaderRow) + 2.54 - 8.509, SpacerZ - 2.25));
        return string.Join("\n", log);
    }
}
