"""Build and verify the photo-layout prototype without Inventor (all lengths mm).

Port of Enclosure.cs using the same generated AsBuiltLayout dimensions. Outputs
are provisional mechanical prototypes, not a claim of measured optical response.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct

import cadquery as cq

from layout import ROOT, CONFIG, csharp, dimensions

C, D = dimensions()
globals().update(D)
FLOOR, LOW, PLATE, BLOCK, CHAMBER, AP_TOP = -17.9, -14.9, -13.4, -5.0, -12.0, -10.5
# Pouch (measured Nov 2026): white reading square flush with the flat face, which
# faces up to the sensors. The bump (ampoules) is on the OTHER face, pointing down,
# starting 1.25" from the white-square (leading) end and ~9 mm tall. Its shape is
# modelled as a full-width envelope from there to the tail.
BUMP_S0, BUMP_H = 31.75, 9.0
POCKET_D, POCKET_LIP = 10.0, 2.0   # floor pocket for the bump; 45 deg lip at its end
PAD_TOP, SPLIT, LID_TOP = FLOOR+.8, 4.0, 29.3
BOTTOM = FLOOR-POCKET_D-2.0
WALL, POCKET_CLEAR, FIT = 2.0, 0.5, 0.2
IN = (-1.5, -6.0, 91.0, 79.5)
OUT = (IN[0]-WALL, IN[1]-WALL, IN[2]+WALL, IN[3]+WALL)
PX0, PX1 = BoardsX0-POCKET_CLEAR, BoardsX1+POCKET_CLEAR
PY0, PY1 = BoardsY0-POCKET_CLEAR, BoardsY1+POCKET_CLEAR
MX0, MX1 = PX0-WALL-FIT, PX1+WALL+FIT
MY0, MY1 = PY0-WALL-FIT, PY1+WALL+FIT
VIEW_EXTRA, LED_D, SLOT_LEN, SLOT_HALF_X, WEB = .4, 1.8, 2.2, 1.5, .8
CHAMBER_TOP_HALF = 1.7   # half-width of the flat chamber ceiling; LED slot is +-1.5
# Strip passage: an entry funnel with headroom through the -Y wall (the only place
# anything spans the strip); inside, open slots. A 0.6-0.7 mm gap closes up when printed.
STRIP_HEADROOM, ENTRY_HALF, ENTRY_TOP = 1.2, 4.0, 1.8
MOD_RIM = 2.0  # module locating rim height/width in the base
# Tunnel Cover: separate flat-printed roof + low shelf over the pouch tunnel, screwed to
# four bosses in the base (M2.5 self-tapping, pilot 2.1 mm, clearance 2.8 mm).
COVER_Z0, COVER_T = LOW, 1.2   # tunnel roof = module underside; only flat film rides above the floor
COVER_SCREWS = [(8,-2.6),(47,-2.6),(5,60.2),(47,60.2)]
POUCH_HALF = [(0,9.03),(21.89,21.92),(34.1,25.4),(81.9,25.4),
              (86.3,23.8),(90.9,19.5),(95.25,15.15)]
LCD_HOLES = [(29.25,72.5),(29.25,-2.5),(60.25,72.5),(60.25,-2.5)]


def view_y(sy, target, z):
    return sy + (target-sy) * (SensorFaceZ-z)/(SensorFaceZ-StripTopZ)


POUCH_Y = view_y(TcsY, TLineY, PAD_TOP)
TIP_X = SensorX + 12.15
POCKET_END = TIP_X-BUMP_S0+.3   # bump front at full insertion + 0.3; the pocket end is the pouch stop


def box(x0, y0, x1, y1, z0, z1):
    if min(x1-x0, y1-y0, z1-z0) <= 0:
        raise ValueError(f"Nonpositive box: {(x0,y0,x1,y1,z0,z1)}")
    return cq.Solid.makeBox(x1-x0, y1-y0, z1-z0, (x0,y0,z0))


def rect_cut(shape, x0, y0, x1, y1, z0, z1):
    return shape.cut(box(x0,y0,x1,y1,z0,z1))


def prism(points, z, h):
    return cq.Workplane("XY").polyline(points).close().extrude(h).val().translate((0,0,z))


def outline(tip, direction, cy, clearance, end=None, slide=False):
    side, widest, end_half = [], 0, None
    for i, (s, half) in enumerate(POUCH_HALF):
        x = tip + direction*s
        if end is not None and direction*(x-end)>0:
            prev_s, prev_h = POUCH_HALF[i-1]
            end_s = direction*(end-tip)
            end_half = prev_h+(half-prev_h)*(end_s-prev_s)/(s-prev_s)
            break
        widest = max(widest, half)
        side.append((x, cy-((widest if slide else half)+clearance)))
    side[0] = (tip-direction*clearance, cy-(POUCH_HALF[0][1]+clearance))
    if end is not None:
        h = cy-side[-1][1]-clearance if end_half is None else end_half
        if slide:
            h = max(h,widest)
        side.append((end,cy-h-clearance))
    return side+[(x,2*cy-y) for x,y in reversed(side)]


def bore(a, b, diameter):
    u = cq.Vector(*b)-cq.Vector(*a)
    return cq.Solid.makeCylinder(diameter/2, u.Length, cq.Vector(*a), u.normalized())


def axis_bore(sy, target, z0, z1, diameter):
    return bore((SensorX,view_y(sy,target,z0),z0),
                (SensorX,view_y(sy,target,z1),z1),diameter)


def read_chamber(y0, y1):
    """Read chamber, open at the module's underside. Its long sides slope at 45 deg so the
    ceiling (aperture plate) is a 3.4 mm span that prints, bottom down, without support."""
    w0, w1 = (ChamberX1-ChamberX0)/2, CHAMBER_TOP_HALF
    zs = CHAMBER-(w0-w1)           # where the 45 deg slope starts
    pts = [(SensorX-w0,LOW-.1),(SensorX+w0,LOW-.1),(SensorX+w0,zs),(SensorX+w1,CHAMBER),
           (SensorX-w1,CHAMBER),(SensorX-w0,zs)]
    return cq.Workplane("XZ").polyline(pts).close().extrude(-(y1-y0)).val().translate((0,y0,0))


def optical_module(aperture):
    p = box(PX0-WALL,PY0-WALL,PX1+WALL,PY1+WALL,LOW,-.2)
    p = rect_cut(p,PX0,PY0,PX1,PY1,PLATE,0)
    p = p.fuse(box(BlockX0,BoardsY0+.3,BlockX1,BoardsY1-.3,PLATE,BLOCK))
    for header in (TcsHeaderY,AsHeaderY):
        p = rect_cut(p,BlockX0-.5,header-1.57,BlockX1+.5,header+1.57,-5.9,BLOCK+.1)
    for y0,y1 in [(ChamberY0,ChamberSplitY-.5),(ChamberSplitY+.5,ChamberY1)]:
        p = p.cut(read_chamber(y0,y1))
    metrics = []
    for tag,sy,target,led in [("TCS3448",TcsY,TLineY,TcsLedY),("AS7343",AsY,CLineY,AsLedY)]:
        cosine = 1/math.sqrt(1+((target-sy)/(StripTopZ-SensorFaceZ))**2)
        # Start 1 mm into the (open) chamber: the cut is tilted, so starting just below the
        # ceiling left a 0-0.16 mm skin over about half the aperture.
        ap = axis_bore(sy,target,CHAMBER-1,AP_TOP+.1,aperture)
        view = axis_bore(sy,target,AP_TOP,BLOCK+.5,aperture+VIEW_EXTRA)
        # The photographed LEDs are on the -Y side of their sensors.
        ap_min = min(view_y(sy,target,AP_TOP),view_y(sy,target,CHAMBER))-aperture/2/cosine
        slot1 = ap_min-WEB
        slot0 = slot1-SLOT_LEN
        slot_c = (slot0+slot1)/2
        slot = box(SensorX-SLOT_HALF_X,slot0,SensorX+SLOT_HALF_X,slot1,CHAMBER,AP_TOP)
        # The bore's tilted end cap stopped part of it short of the slot. Square the end off
        # at the slot ceiling over the slot, but stop 0.3 mm short of the slot's aperture-side
        # edge so the LED bore stays as far from the view bore as the slot already is.
        u = cq.Vector(0,slot_c-led,AP_TOP-LedZ).normalized()
        led_bore = bore((SensorX,led,LedZ),(SensorX,slot_c,AP_TOP),LED_D)
        led_bore = led_bore.fuse(bore((SensorX,led,LedZ),(SensorX,slot_c+1.5*u.y,AP_TOP+1.5*u.z),LED_D)
                                 .intersect(box(SensorX-5,led-20,SensorX+5,slot1-.3,AP_TOP,LedZ+5)))
        # CAD-kernel distance and Boolean overlap in the optical block, not only a centreline estimate.
        block_region = box(BlockX0,BoardsY0,BlockX1,BoardsY1,CHAMBER,BLOCK)
        view_cut = ap.fuse(view).intersect(block_region)
        light_cut = slot.fuse(led_bore).intersect(block_region)
        gap = view_cut.distance(light_cut)
        overlap = view_cut.intersect(light_cut).Volume()
        if gap < .4 or overlap > 1e-6:
            raise ValueError(f"{tag} d{aperture}: bore isolation fails, gap={gap}, overlap={overlap}")
        p = p.cut(ap).cut(view).cut(slot).cut(led_bore)
        # Envelope based on the lower slot lip and finite LED width. This alone
        # is NOT proof of illumination: upper bore and chamber can clip rays.
        def envelope(zt):
            rays = [e+(e-l)*(CHAMBER-zt)/(LedZ-CHAMBER)
                    for e in (slot0,slot1) for l in (led-1.4,led+1.4)]
            return [min(rays),max(rays)]
        metrics.append(dict(sensor=tag,line_y=target,view_at_pad_y=view_y(sy,target,PAD_TOP),
                            led_y=led,slot_y=[slot0,slot1],minimum_bore_gap_mm=gap,
                            bore_overlap_mm3=overlap,strip_envelope_y=envelope(StripTopZ),
                            pad_envelope_y=envelope(PAD_TOP)))
    return p.clean(), metrics


def check_optical_rays(module):
    """Straight, unreflected paths through actual solid; not a radiometric model.

    Nine sample points on an assumed 2 x 2 mm emitter are tested against each
    target centre. Require a clear view axis and at least one clear LED ray.
    This catches a plugged bore / wrong-way slot but does not certify uniformity,
    signal strength, footprint, glossy-film reflection or crosstalk.
    """
    checks = {}
    for name,sy,led,target,z in [
        ("TCS_test",TcsY,TcsLedY,TLineY,StripTopZ),
        ("AS_control",AsY,AsLedY,CLineY,StripTopZ),
        ("TCS_color",TcsY,TcsLedY,POUCH_Y,PAD_TOP)]:
        end = (SensorX,target,z)
        view = cq.Edge.makeLine((SensorX,sy,SensorFaceZ),end)
        view_blocked = sum(e.Length() for e in module.intersect(view).Edges())
        clear = 0
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                ray = cq.Edge.makeLine((SensorX+dx,led+dy,LedZ-.01),end)
                if sum(e.Length() for e in module.intersect(ray).Edges()) < 1e-5:
                    clear += 1
        if view_blocked > 1e-5 or clear == 0:
            raise ValueError(f"Blocked optical path {name}: view={view_blocked}, clear LED rays={clear}")
        checks[name] = {"view_axis_blocked_mm":view_blocked,"clear_led_sample_rays":clear,"led_sample_rays":9}
    return checks


def bump_pocket():
    """Full-width floor pocket for the downward bump, open through the entrance."""
    lane = prism(outline(TIP_X,-1,POUCH_Y,.5,OUT[0]-2,True),FLOOR-POCKET_D-1,POCKET_D+2)
    p = lane.intersect(box(OUT[0]-3,-30,POCKET_END,120,FLOOR-POCKET_D,FLOOR+.01))
    # 45 deg lip on the pocket's far edge: the flat leading end rides up onto the
    # floor; the bump's front face meets the vertical part below it and stops.
    lip = (cq.Workplane("XZ").polyline([(POCKET_END,FLOOR-POCKET_LIP),(POCKET_END,FLOOR+.01),
                                         (POCKET_END+POCKET_LIP,FLOOR+.01)]).close()
           .extrude(-200).val().translate((0,-50,0)))
    return p.fuse(lip.intersect(lane))


def base():
    x0,y0,x1,y1 = OUT
    p = box(x0,y0,x1,y1,BOTTOM,SPLIT)
    p = rect_cut(p,*IN,FLOOR,SPLIT+.1)
    # Cut the insertion envelope through the corner supports as well.
    for r in [(IN[0],IN[1],3.5,4.25),(86.5,IN[1],IN[2],4.25),
              (IN[0],66.5,3.5,IN[3]),(86.5,66.5,IN[2],IN[3])]:
        p = p.fuse(box(*r,FLOOR,0))
    p = p.fuse(box(MX0,MY0,MX1,MY1,FLOOR,LOW))
    # Pouch tunnel walls only, up to the cover's underside. The roof and low shelf are the
    # separate Tunnel Cover, so nothing in the base spans the ~51 mm tunnel.
    p = p.fuse(prism(outline(TIP_X,-1,POUCH_Y,2.5,x0,True),FLOOR,COVER_Z0-FLOOR))
    # Locating rim (as on the sled), standing on the floor wherever the pouch doesn't run:
    # full +X and +Y sides, -Y from where the slot narrows, and a -X stop post at the +Y
    # corner. The cover's shelf end is the main -X stop.
    p = p.fuse(box(MX0-MOD_RIM,MY0-MOD_RIM,IN[2],MY1+MOD_RIM,FLOOR,LOW+MOD_RIM))
    p = p.cut(prism(outline(TIP_X,-1,POUCH_Y,.5,x0-2,True),FLOOR,LOW+MOD_RIM+.1-FLOOR))
    p = p.cut(prism(outline(TIP_X,-1,POUCH_Y,.5,x0-2,True),FLOOR,COVER_Z0-FLOOR))
    # Entrance: open the -X wall 2 mm above the cover's underside. The cover's front lip
    # fills it, so any sag in the wall's bridge over the entrance lands on the cover.
    slot = prism(outline(TIP_X,-1,POUCH_Y,.5,x0-2,True),FLOOR,SPLIT-FLOOR)
    p = p.cut(slot.intersect(box(x0-1,-10,IN[0]+.1,90,COVER_Z0,COVER_Z0+2)))
    # No corner-post stub hanging over the tunnel.
    p = p.cut(slot.intersect(box(IN[0]-.1,IN[1]-.1,3.6,4.35,FLOOR,.1)))
    for x,y in COVER_SCREWS:
        p = p.fuse(cq.Solid.makeCylinder(3,COVER_Z0-FLOOR,cq.Vector(x,y,FLOOR)))
        p = p.cut(cq.Solid.makeCylinder(1.05,7,cq.Vector(x,y,COVER_Z0-7)))
    p = rect_cut(p,MX0,MY0,MX1,MY1,LOW,SPLIT+.1)
    gx0,gx1 = SensorX-StripW/2-.2,SensorX+StripW/2+.2
    # Insertion is now from -Y: TCS=test at the lower Y and AS=control above it.
    p = rect_cut(p,gx0,y0-1,gx1,StripStopY+.3,FLOOR-StripT-.2,FLOOR)
    p = rect_cut(p,gx0,y0-1,gx1,IN[1]+.1,FLOOR,FLOOR+STRIP_HEADROOM)
    # Inside, nothing spans the strip: walls it crosses get an open slot down to the floor.
    p = rect_cut(p,gx0,IN[1],gx1,StripStopY+.3,FLOOR,LOW+MOD_RIM+.1)
    # No -Y rim on the -X side of the strip slot: there the pouch slot's angled edge leaves
    # only a <0.3 mm wedge of it on top of the tunnel wall.
    p = rect_cut(p,MX0-MOD_RIM-.1,MY0-MOD_RIM-.1,gx1,MY0,LOW,LOW+MOD_RIM+.1)
    # Funnel: 8 mm wide at the outside face, tapering ~45 deg to the groove at the inside face.
    p = p.cut(prism([(SensorX-ENTRY_HALF,y0-1),(SensorX+ENTRY_HALF,y0-1),(SensorX+ENTRY_HALF,y0),
                     (gx1,IN[1]+.1),(gx0,IN[1]+.1),(SensorX-ENTRY_HALF,y0)],
                    FLOOR-StripT-.2,StripT+.2+ENTRY_TOP))
    p = rect_cut(p,23.41-6,IN[3]-.1,23.41+6,y1+.5,-9.4,-1.4)
    p = rect_cut(p,x0-1,POUCH_Y-10,8,POUCH_Y+10,BOTTOM-.5,FLOOR)
    p = rect_cut(p,x0-1,POUCH_Y-10,8,POUCH_Y+10,COVER_Z0,COVER_Z0+6)
    p = p.cut(bump_pocket())
    return p.clean()


def cover():
    """Flat roof of the pouch tunnel (3 mm above the floor); prints flat, no supports."""
    x0 = OUT[0]
    p = prism(outline(TIP_X,-1,POUCH_Y,2.5,x0,True),COVER_Z0,COVER_T)
    for x,y in COVER_SCREWS:
        # Ear from the tunnel roof out to the boss, kept 0.2 mm inside the base walls.
        y0,y1 = (max(IN[1]+.2,y-3.5),1.5) if y < POUCH_Y else (POUCH_Y+27,min(IN[3]-.2,y+3.5))
        p = p.fuse(box(x-4,y0,x+4,y1,COVER_Z0,COVER_Z0+COVER_T))
    inner = prism(outline(TIP_X,-1,POUCH_Y,.3,x0-2,True),LOW-1,10)
    p = p.cut(box(x0-1,-10,IN[0]+.2,90,LOW-1,0))            # base -X wall
    p = p.fuse(inner.intersect(box(x0,-10,IN[0]+.2,90,COVER_Z0,COVER_Z0+COVER_T)))  # front lip in the wall opening
    p = p.cut(box(MX0-MOD_RIM-.2,-30,200,200,LOW-1,0))      # module and its rim
    p = p.cut(box(IN[0]-1,IN[1]-1,3.7,4.45,LOW-1,0).cut(inner))  # corner board post
    p = p.cut(box(x0-1,POUCH_Y-10,8,POUCH_Y+10,LOW-1,0))     # finger notch above the pouch
    for x,y in COVER_SCREWS:
        p = p.cut(cq.Solid.makeCylinder(1.4,4,cq.Vector(x,y,COVER_Z0-1)))
    return p.clean()


def lid():
    p = box(*OUT,SPLIT,LID_TOP)
    p = rect_cut(p,*IN,SPLIT,LID_TOP-WALL)
    lx0,ly0,lx1,ly1 = IN[0]+.2,IN[1]+.2,IN[2]-.2,IN[3]-.2
    p = p.fuse(box(lx0,ly0,lx1,ly1,2,6)).fuse(box(*IN,4,6))
    p = rect_cut(p,lx0+1.2,ly0+1.2,lx1-1.2,ly1-1.2,2,10)
    p = rect_cut(p,32.35,-.9,57.15,70.9,27.3,29.4)
    for x,y in LCD_HOLES:
        p = p.fuse(cq.Solid.makeCylinder(3,5.2,(x,y,22.1)))
        p = p.cut(cq.Solid.makeCylinder(1.25,4,(x,y,22.1)))
    for r in [(0,0,3.5,3.5),(86.5,0,90,3.5),(0,66.5,3.5,70),(86.5,66.5,90,70)]:
        p = p.fuse(box(*r,1.7,27.3))
    return p.clean()


def sled():
    p = box(0,0,90,70,BOTTOM,FLOOR)
    p = p.fuse(box(MX0-2,MY0-2,MX1+2,MY1+2,BOTTOM,LOW+2))
    p = rect_cut(p,MX0,MY0,MX1,MY1,LOW,LOW+2.1)
    p = p.cut(prism(outline(TIP_X,-1,POUCH_Y,.5,MX0-4,True),FLOOR,3))
    # -X rim: cut only where the pouch slot actually runs, leaving a stop post at the +Y
    # corner. The -Y corner sits over the slot, so it can't have one without a bridge.
    # No rim anywhere over the slot (it would hang there), leaving a -X stop post at the +Y
    # corner. The -Y corner sits over the slot, so it can't have one without a bridge.
    p = p.cut(prism(outline(TIP_X,-1,POUCH_Y,.5,MX0-4,True),LOW,2.1))
    p = rect_cut(p,SensorX-StripW/2-.2,-1,SensorX+StripW/2+.2,StripStopY+.3,FLOOR-StripT-.2,FLOOR)
    # Open slot through everything the strip crosses: nothing spans it.
    p = rect_cut(p,SensorX-StripW/2-.2,-1,SensorX+StripW/2+.2,StripStopY+.3,FLOOR,LOW+2.1)
    # Between the strip slot and the pouch slot's angled edge only a thin wedge would remain.
    p = rect_cut(p,MX0-2.1,MY0-2.1,SensorX+StripW/2+.2,MY0,FLOOR,LOW+2.1)
    for r in [(0,0,4,4),(86,0,90,4),(0,66,4,70),(86,66,90,70)]:
        p = p.fuse(box(*r,FLOOR,0))
    p = p.cut(prism(outline(TIP_X,-1,POUCH_Y,.5,-2,True),FLOOR,3))
    p = p.cut(bump_pocket())
    return p.clean()


def gauge():
    # A 2 mm alignment template in the perfboard frame. Two breakout windows,
    # corner registration holes, and +Y USB notch make orientation unambiguous.
    p = box(0,0,90,70,0,2)
    p = rect_cut(p,5,5,BoardsX0-3,65,-.1,2.1)
    for sy in (TcsY,AsY):
        p = rect_cut(p,BoardsX0-.5,sy-(17.78-8.509)-.5,BoardsX1+.5,sy+8.509+.5,-.1,2.1)
    # The nominal 1.22 mm gap cannot retain a sturdy web: join the two windows.
    p = rect_cut(p,BoardsX0-.5,TcsY,BoardsX1+.5,AsY,-.1,2.1)
    for x in (2.5,87.5):
        for y in (2.5,67.5):
            p = p.cut(cq.Solid.makeCylinder(1.1,3,(x,y,-.5)))
    p = rect_cut(p,18,66,28,71,-.1,2.1)
    return p.clean()


def strip():
    p = box(SensorX-StripW/2,StripStopY-StripLen,SensorX+StripW/2,StripStopY,StripTopZ-StripT,StripTopZ)
    for y in (TLineY,CLineY):
        p = p.fuse(box(SensorX-StripW/2,y-.5,SensorX+StripW/2,y+.5,StripTopZ,StripTopZ+.05))
    return p.clean()


def pouch():
    p = prism(outline(TIP_X,-1,POUCH_Y,0),FLOOR,.8)   # white square flush on the top face
    bump = prism(outline(TIP_X,-1,POUCH_Y,0),FLOOR-BUMP_H,BUMP_H+.01)
    return p.fuse(bump.intersect(box(TIP_X-100,-30,TIP_X-BUMP_S0,120,FLOOR-BUMP_H-1,FLOOR+1))).clean()


def load_electronics():
    """Actual vendor breakout shape; AS uses the inherited TCS mechanical proxy."""
    vendor = ROOT / "Purchased Parts/Adafruit/6525 TCS3448 Light Color Sensor.step"
    shape = cq.importers.importStep(str(vendor)).val()
    header = cq.importers.importStep(str(ROOT/"reference/Header.step")).val()
    parts = {}
    # local x -> +X, y -> -Y, z -> -Z: 180 deg about X, determinant +1.
    for name,sy in [("TCS3448_test_color",TcsY),("AS7343_control_proxy",AsY)]:
        offset = (SensorX-12.7,sy+8.509,SpacerZ)
        parts[name] = shape.rotate((0,0,0),(1,0,0),180).translate(offset)
        parts[name+"_header"] = header.rotate((0,0,0),(1,0,0),180).translate(offset)
    for name in ("ESP32","LCD"):
        parts[name] = cq.importers.importStep(str(ROOT/f"reference/{name}.step")).val()
    # Outline reference only: the photo grid is not established solder metrology.
    parts["Perfboard_outline_reference"] = box(0,0,90,70,0,1.6)
    return parts


def check_stl(path):
    data = path.read_bytes()
    n = struct.unpack_from("<I",data,80)[0]
    if len(data) != 84+50*n:
        raise ValueError(f"Expected binary STL: {path}")
    edges = Counter()
    for i in range(n):
        xyz = struct.unpack_from("<12f",data,84+i*50)[3:]
        pts = [tuple(round(v,5) for v in xyz[j:j+3]) for j in (0,3,6)]
        if len(set(pts)) != 3:
            raise ValueError(f"Degenerate STL facet: {path}")
        for a,b in [(0,1),(1,2),(2,0)]:
            edges[tuple(sorted((pts[a],pts[b])))] += 1
    bad = sum(v!=2 for v in edges.values())
    if bad:
        raise ValueError(f"STL not closed two-manifold: {path}: {bad} edges")
    return {"triangles":n,"nonmanifold_edges":bad}


def export_part(name,shape,out):
    if not shape.isValid() or len(shape.Solids()) != 1 or shape.Volume() <= 0:
        raise ValueError(f"Invalid printable solid: {name}, {len(shape.Solids())} solids")
    step_path,stl_path = out/f"{name}.step",out/f"{name}.stl"
    cq.exporters.export(shape,str(step_path))
    shape.exportStl(str(stl_path),tolerance=.01,angularTolerance=.05,ascii=False)
    back = cq.importers.importStep(str(step_path)).val()
    if not back.isValid() or abs(back.Volume()-shape.Volume())>1e-3:
        raise ValueError(f"STEP round-trip failed: {name}")
    bb = shape.BoundingBox()
    return {"valid":True,"solids":1,"volume_mm3":shape.Volume(),
            "bounds_mm":[bb.xmin,bb.ymin,bb.zmin,bb.xmax,bb.ymax,bb.zmax],
            "step_volume_error_mm3":abs(back.Volume()-shape.Volume()),**check_stl(stl_path)}


def check_fit(parts, module, strip_shape, pouch_shape, electronics):
    checks = {}
    # Pouch and strip are alternative uses, never interference-tested together.
    for fixture in ("Enclosure Base","Test Sled"):
        fixed = parts[fixture]
        if fixture == "Enclosure Base":
            fixed = fixed.fuse(parts["Enclosure Lid"]).fuse(parts["Tunnel Cover"])
        fixed = cq.Compound.makeCompound([fixed,module])
        for name,shape,axis,travel in [("strip",strip_shape,(0,-1,0),80),("pouch",pouch_shape,(-1,0,0),95)]:
            maximum = 0
            for mm in range(travel+1):
                moving = shape.translate(tuple(mm*v for v in axis))
                vol = moving.intersect(fixed).Volume()
                maximum = max(maximum,vol)
                if vol>1e-4:
                    raise ValueError(f"{fixture}/{name} collision at {mm} mm: {vol} mm3")
            checks[f"{fixture}/{name}"] = {"positions":travel+1,"max_overlap_mm3":maximum}
    for name in ("TCS3448_test_color","AS7343_control_proxy"):
        vol = module.intersect(electronics[name]).Volume()
        pinvol = module.intersect(electronics[name+"_header"]).Volume()
        if max(vol,pinvol)>1e-4:
            raise ValueError(f"Module hits {name}: {vol}, header {pinvol} mm3")
        checks[f"module/{name}"] = {"board_overlap_mm3":vol,"header_overlap_mm3":pinvol}
    for a,b in [("Enclosure Base","Enclosure Lid"),("Enclosure Base","Tunnel Cover"),("Tunnel Cover","Enclosure Lid")]:
        vol = parts[a].intersect(parts[b]).Volume()
        if vol>1e-4:
            raise ValueError(f"{a}/{b} interference: {vol}")
        checks[f"{a}/{b}"] = {"overlap_mm3":vol}
    for name in ("ESP32","LCD","TCS3448_test_color","AS7343_control_proxy",
                 "TCS3448_test_color_header","AS7343_control_proxy_header"):
        vol = parts["Tunnel Cover"].intersect(electronics[name]).Volume()
        if vol>1e-4:
            raise ValueError(f"Tunnel Cover hits {name}: {vol} mm3")
        checks[f"Tunnel Cover/{name}"] = {"overlap_mm3":vol}
    for name in ("Enclosure Base","Tunnel Cover","Test Sled"):
        vol = module.intersect(parts[name]).Volume()
        if vol>1e-4:
            raise ValueError(f"Module/{name} interference: {vol}")
        checks[f"module/{name}"] = {"overlap_mm3":vol}
    return checks


def assembly(path,shapes):
    a = cq.Assembly(name=path.stem.replace(" ","_"))
    for name,s in shapes.items():
        if ("TCS3448" in name or "AS7343" in name) and not name.endswith("_header"):
            # Viewer assemblies omit thin copper/mask and sub-mm3 passives.
            # Full vendor solids are still used for the interference checks.
            s = cq.Compound.makeCompound([v for v in s.Solids()
                                          if v.Volume() >= 1 and v.BoundingBox().zlen > .2])
        color = (.22,.23,.25)
        if "TCS" in name:
            color = (.10,.60,.75)
        elif "AS7343" in name:
            color = (.9,.55,.15)
        elif "Perfboard" in name:
            color = (.12,.44,.27)
        elif name in ("LFA Strip","Pouch"):
            color = (.85,.83,.77)
        a.add(s,name=name.replace(" ","_"),color=cq.Color(*color))
    a.save(str(path),exportType="STEP")


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--out",type=Path,default=ROOT/"exports/as-built")
    args = parser.parse_args()
    if (ROOT/"scripts/AsBuiltLayout.cs").read_text() != csharp(D):
        raise SystemExit("Run scripts/layout.py to refresh shared C# constants")
    args.out.mkdir(parents=True,exist_ok=True)
    report = {"revision":C["revision"],"status":C["status"],"cadquery":cq.__version__,
              "layout_sha256":hashlib.sha256(CONFIG.read_bytes()).hexdigest(),
              "parts":{},"optics":{},"fit":{},
              "not_verified":["physical XY or heights", "native Inventor execution", "optical signal or calibration", "real wire routes", "AS7343 proxy component heights"]}
    report["assembly_detail"] = "Breakout exports omit copper/mask and solids below 1 mm3 or 0.2 mm tall; full vendor solids used for interference. Perfboard is outline only."
    report["source_sha256"] = {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in [CONFIG,ROOT/"scripts/layout.py",Path(__file__),
                                         ROOT/"scripts/Enclosure.cs",ROOT/"scripts/Electronics.cs",
                                         ROOT/"scripts/LfaStrip.cs",ROOT/"scripts/Wiring.cs",
                                         ROOT/"reference/ESP32.step",ROOT/"reference/LCD.step",
                                         ROOT/"reference/Header.step",
                                         ROOT/"Purchased Parts/Adafruit/6525 TCS3448 Light Color Sensor.step"]}
    parts = {"Enclosure Base":base(),"Tunnel Cover":cover(),"Enclosure Lid":lid(),"Test Sled":sled(),"Alignment Gauge":gauge()}
    report["ray_checks"] = {}
    for ap in C["apertures_mm"]:
        name = f"Optical Module d{ap:.1f}"
        parts[name],report["optics"][name] = optical_module(ap)
        report["ray_checks"][name] = check_optical_rays(parts[name])
    for name,part in parts.items():
        print(f"Export/check {name}",flush=True)
        report["parts"][name] = export_part(name,part,args.out)
    print("Load vendor and reference electronics",flush=True)
    electronics = load_electronics()
    default = parts[f"Optical Module d{ApertureD:.1f}"]
    strip_shape,pouch_shape = strip(),pouch()
    print("Check insertion sweeps, body and pin clearances",flush=True)
    report["fit"] = check_fit(parts,default,strip_shape,pouch_shape,electronics)
    print("Export assemblies (pouch and LFA are separate configurations)",flush=True)
    assembly(args.out/"Electronics Assembly.step",electronics)
    for mode,sample in [("LFA",{"LFA Strip":strip_shape}),("Color",{"Pouch":pouch_shape})]:
        assembly(args.out/f"Enclosure {mode}.step",dict(electronics,**{
            "Enclosure Base":parts["Enclosure Base"],"Tunnel Cover":parts["Tunnel Cover"],"Optical Module":default,
            "Enclosure Lid":parts["Enclosure Lid"]},**sample))
        assembly(args.out/f"Test Jig {mode}.step",dict(electronics,**{
            "Test Sled":parts["Test Sled"],"Optical Module":default},**sample))
    report["sha256"] = {p.name:hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in sorted(args.out.iterdir()) if p.suffix in (".stl",".step")}
    if args.out.resolve() == (ROOT/"exports/as-built").resolve():
        # Keep existing GitHub/viewer/print entry points current. Native .iam/.ipt
        # remain legacy until the separate Inventor rebuild; README states this.
        mirrors = {"Enclosure LFA.step":ROOT/"Enclosure Assembly.step",
                   "Electronics Assembly.step":ROOT/"Electronics Assembly.step"}
        for name in parts:
            if name == "Test Sled" or name.startswith("Optical Module"):
                for ext in ("step","stl"):
                    mirrors[f"{name}.{ext}"] = ROOT/"exports/test-jig"/f"{name}.{ext}"
        for src,dest in mirrors.items():
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(args.out/src,dest)
        report["compatibility_copies"] = {dst.relative_to(ROOT).as_posix():src for src,dst in mirrors.items()}
    (args.out/"validation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(f"Verified {len(parts)} printable solids; report: {args.out/'validation.json'}",flush=True)


if __name__ == "__main__":
    main()
