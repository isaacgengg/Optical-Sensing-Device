// Pin-net markers for the photo layout, NOT a model of the unseen underside wiring.
// Sensor positions are provisional; connect by silkscreen and verify continuity.
using System;
using System.Collections.Generic;
using Inventor;

public static class Wiring
{
    public static string Build(object appObj, string path)
    {
        var app = (Application)appObj;
        var tg = app.TransientGeometry;
        if (System.IO.File.Exists(path)) throw new Exception("Refusing to overwrite " + path);
        var tpl = app.FileManager.GetTemplateFile(DocumentTypeEnum.kPartDocumentObject, SystemOfMeasureEnum.kMetricSystemOfMeasure);
        var doc = (PartDocument)app.Documents.Add(DocumentTypeEnum.kPartDocumentObject, tpl, false);
        var cd = doc.ComponentDefinition;
        var log = new List<string>();
        string[] nets = { "3V3 to VIN", "GND", "SCL D22", "SDA D21" };
        string[] looks = { "Red", "Black", "Yellow", "Orange" };
        double[] rows = { AsBuiltLayout.TcsHeaderY, AsBuiltLayout.AsHeaderY };
        string[] sensors = { "TCS3448", "AS7343" };
        for (int s = 0; s < 2; s++) for (int pin = 0; pin < 4; pin++)
        {
            double x = AsBuiltLayout.SensorX - 6.35 + 2.54 * pin, y = rows[s];
            var wp = cd.WorkPlanes.AddByPlaneAndOffset(cd.WorkPlanes[3], 0.2);
            wp.Visible = false;
            var sk = cd.Sketches.Add(wp);
            sk.SketchCircles.AddByCenterRadius(tg.CreatePoint2d(x / 10, y / 10), 0.05);
            var ed = cd.Features.ExtrudeFeatures.CreateExtrudeDefinition(sk.Profiles.AddForSolid(), PartFeatureOperationEnum.kNewBodyOperation);
            ed.SetDistanceExtent(0.08, PartFeatureExtentDirectionEnum.kPositiveExtentDirection);
            var f = cd.Features.ExtrudeFeatures.Add(ed);
            f.Name = sensors[s] + " " + nets[pin]; sk.Visible = false;
            foreach (AssetLibrary lib in app.AssetLibraries) foreach (Asset a in lib.AppearanceAssets)
            {
                if (a.DisplayName != looks[pin]) continue;
                Asset local = null;
                foreach (Asset b in doc.AppearanceAssets) if (b.DisplayName == a.DisplayName) local = b;
                if (local == null) local = a.CopyTo(doc, false);
                f.Appearance = local;
            }
            log.Add(string.Format("{0} at XY ({1:F3}, {2:F3}) -- PROVISIONAL", f.Name, x, y));
        }
        doc.PropertySets["Design Tracking Properties"]["Description"].Value =
            "Provisional sensor pin-net markers only; no wire routing or continuity claim. See hardware/WIRING.md.";
        doc.SaveAs(path, false); doc.Close(true);
        return string.Join("\n", log);
    }

    public static string Check(object appObj, string path)
    {
        return "Pin markers only. Actual wire routes are not visible in the supplied component-side photos; verify continuity physically.";
    }
}
