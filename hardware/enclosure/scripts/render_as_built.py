"""Render a dimensioned comparison and a section of the generated CAD, not a photo edit."""
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib.collections import PolyCollection
import cadquery as cq
import build_as_built as b

OUT = b.ROOT / "exports/as-built"
TCS, AS, LED = "#087f9c", "#bd651a", "#f1bd31"
plt.rcParams.update({"font.size":10,"font.family":"DejaVu Sans"})


def plan():
    fig,axes = plt.subplots(1,2,figsize=(12,6.5),layout="constrained")
    cases = [("Previous CAD",70.4,34.62,16.84,1),
             ("Photo-derived layout (provisional)",b.SensorX,b.TcsY,b.AsY,-1)]
    for ax,(title,x,ty,ay,sign) in zip(axes,cases):
        ax.add_patch(Rectangle((0,0),90,70,fc="#eaf2ee",ec="#759286"))
        ax.add_patch(Rectangle((90-37.66,15.6),28.5,52.5,fc="#39434d",ec="none"))
        ax.text(90-23.41,42,"ESP32",color="white",ha="center",rotation=90)
        ax.text(90-23.41,72,"USB / +Y",ha="center",weight="bold")
        for name,y,color in [("TCS · test/color",ty,TCS),("AS · control",ay,AS)]:
            low = y-8.509 if sign==1 else y-(17.78-8.509)
            ax.add_patch(Rectangle((90-x-12.7,low),25.4,17.78,fc=color,alpha=.15,ec=color,lw=1.5))
            ax.plot(90-x,y,"o",ms=7,color=color)
            ly = y+sign*(15.875-8.509)
            hy = y-sign*(8.509-2.54)
            ax.scatter([90-x],[ly],marker="s",s=90,c=LED,edgecolors="#9b7510",zorder=4)
            ax.plot([90-x-6.35,90-x+6.35],[hy,hy],"k-",lw=2)
            ax.text(90-x+16,y,name,va="center",color=color,fontsize=9,
                    bbox=dict(facecolor="white",edgecolor="none",alpha=.9,pad=1))
        ax.annotate("",(90-x-16,ty),(90-x-16,ay),arrowprops=dict(arrowstyle="<->",color="#56606b"))
        ax.text(90-x-18,(ty+ay)/2,f"{abs(ty-ay):.2f} mm",rotation=90,ha="right",va="center",fontsize=9)
        ax.set(title=title,xlim=(-7,97),ylim=(-5,78),aspect="equal")
        ax.set_xticks([]);ax.set_yticks([])
        for spine in ax.spines.values():spine.set_visible(False)
    fig.suptitle("Same electrical roles • boards reversed and shifted",fontsize=17,weight="bold")
    fig.supxlabel("Component-side view, USB at top. Yellow = LED; black bar = header.\nCAD X is mirrored in this view. Photo dimensions need a physical fit check.",fontsize=10)
    fig.savefig(OUT/"layout-comparison.png",dpi=160)
    plt.close(fig)


def section():
    m = cq.importers.importStep(str(OUT/"Optical Module d1.6.step")).val()
    thin = m.intersect(b.box(b.SensorX-.01,b.MY0-1,b.SensorX+.01,b.MY1+1,b.LOW-1,1))
    vertices,faces = thin.tessellate(.01,.05)
    polygons = [[(vertices[i].y,vertices[i].z) for i in f] for f in faces]
    fig,ax = plt.subplots(figsize=(12,6.5),layout="constrained")
    ax.add_collection(PolyCollection(polygons,facecolors="#333a43",edgecolors="none"))
    for tag,sy,led,target,color in [("TCS · TEST / COLOR",b.TcsY,b.TcsLedY,b.TLineY,TCS),
                                    ("AS · CONTROL",b.AsY,b.AsLedY,b.CLineY,AS)]:
        ax.plot([sy,target],[b.SensorFaceZ,b.StripTopZ],color=color,lw=2)
        ax.scatter([sy],[b.SensorFaceZ],s=70,color=color,zorder=5)
        ax.scatter([led],[b.LedZ],s=100,marker="s",c=LED,edgecolors="#9b7510",zorder=5)
        ax.plot([led,target],[b.LedZ,b.StripTopZ],color=LED,lw=1.5,ls="--")
        ax.text(sy,-2.7,tag,ha="center",color=color,weight="bold",fontsize=10)
    ax.plot([b.StripStopY-b.StripLen,b.StripStopY],[b.StripTopZ,b.StripTopZ],color="#b7aa8d",lw=4)
    for label,y,color in [("T",b.TLineY,TCS),("C",b.CLineY,AS)]:
        ax.plot([y-.5,y+.5],[b.StripTopZ+.03]*2,color="#a72d48",lw=4)
        ax.text(y,-19.3,f"{label}  Y={y:.1f}",ha="center",color=color,weight="bold")
    ax.annotate("LFA enters from −Y",(22,-20.8),(15,-20.8),arrowprops=dict(arrowstyle="->"),fontsize=10)
    ax.text(b.StripStopY,-16.9,"tip stop",ha="center",fontsize=9)
    ax.scatter([b.POUCH_Y],[b.PAD_TOP],s=75,marker="x",c=TCS,zorder=6)
    ax.text(b.POUCH_Y-.5,b.PAD_TOP+.7,"color pad target",ha="right",color=TCS,fontsize=9)
    ax.set(xlim=(b.MY0-2,b.MY1+2),ylim=(-22,0),aspect="equal",
           xlabel="Y in CAD frame (mm)",ylabel="Z below perfboard (mm)",
           title="Actual Ø1.6 mm module section at X = 73 mm")
    ax.grid(alpha=.12)
    fig.supxlabel("Dark = printed material • solid lines = view axes • dashed = LED centre rays\nTargets shown at their separate LFA / colorimetric heights; illumination strength is unverified.",fontsize=9)
    fig.savefig(OUT/"optical-section.png",dpi=160)
    plt.close(fig)


if __name__ == "__main__":
    plan()
    section()
    print(OUT/"layout-comparison.png")
    print(OUT/"optical-section.png")
