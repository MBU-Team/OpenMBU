==============================================================================

This is the exporter DLL for the MilkShape 3D modeler available
from chUmbaLum sOft : //www.swissquake.ch/chumbalum-soft/

The original version of this code was written be José Luis Cebrián who
has graciously allowed us to integrate his code into the Torque SDK.
José did an awesome job of putting this tool together and the GG staff
is very gratefull for the work he's put in to it.

If you look through the code, you'll notice a little complexity in the
DTS file format. A little background is in order... the dts format was
originally designed to reduce load time processing. This means that the
DTS format is almost a direct representation of how the TS engine stores
data in memory.   This does reduce load times, but also ties the file
format to the run-time implementation, which is not really a good idea.
Changes in run-time implementation, and the pre-processing needed for
the run-time data, resulted in un-planned changes and re-structuring of
the DTS file format. These changes leave us with a file format which
leaves much to be desired.

To help reduce the complications involved in writing new exporters,
the MilkShape exporter is divided into a base DTS "SDK", which provides
the core support needed to deal with DTS files, and a small set of
MilkShape specific files which do the actual conversion.  The long term
goal is split this DTS SDK into a seperate library to be shared between
exporter projects.


Current Functionality
---------------------

The exporter is currently in it's 1.0 "beta" phase. Which means it's
usable. Though there is much functinality to add, and several outstanding
issues.  Maybe "alpha" might be more appropriate :)

- The exporter only exports diffuse texture materials.
- Multi-sequence animations
- Animation only supports bone animation (no texture, texture coor,
  vertex morphing or mesh visibility)
- Support for single collision mesh
- Named nodes.


Exporter Flags & Animation Sequences
------------------------------------

Since MilkShape does not directly support a number of Torque engine features
so the tool has been extended through the use of "hacks".  These are described
more fully below but essentially fall into two categories: mesh flags embeded
in the mesh's name, and specially named materials which are used to declare
animation sequences and exporter options.


Multi-Sequence Animations
-------------------------

Material with special names can be used to declare sequence information.
These materials are ignored during export and are solely used to declare
animation sequences.  Sequence materials are named as follows:

	seq: option, option, ...

All other properties of the material are ignored.  The following options are recognized:

        name=start-end This declares the name of the sequence followed by
                       the starting and ending key frames.  This option must
                       exist for the sequence declaration to be valid. 
        fps=n          The number of frames/second. This value affects the
                       duration and playback speed of the sequence.
        cyclic         Sequences are non-cyclic by default. Cyclic animations
                       automatically loop back to the start and never end.

Examples of valid sequence declarations:
 
        "seq: fire=1-4"
        "seq: rotate=5-8, cyclic, fps=2"
	"seq: reload=9-12, fps=5"


Setting Export Options
----------------------

Materials with special names can be used to set several export options.
These materials are ignored during export and are solely used to set otoins.  Option materials are named as follows:

	opt: option, option, ...

All other properties of the material are ignored.  The following options are recognized:

	scale=n         Shape scale factor, where n is a floating point value.
                        The default scale value is 0.1
        size=n          The minimum visible pixel size, default is 0
        fps=n           The default frames/second value for animations. Each
                        animation sequence may set this value, but if it's
                        not defined by the sequence, this default value is used.
        cyclic          The default animation looping flag.  Each animation
                        sequence may set this value, but if it's not defined
                        by the sequence, this default value is used.

There may be more than one option material.  If the same options are set on mulitple materials, then the last one in the material list is the value used.
Examples of valid material names:

        "opt: fps=10, cyclic"
        "opt: scale=1"


Mesh Option Flags
-----------------

Mesh may have additional flags embedded in the mesh (or group) name. The mesh name follows the following format:

        name: flag, flag, ...

where the : and flags are optional.  The following flags are recognized:

        Billboard       The mesh always faces the viewer
        BillboardZ      The mesh faces the viewer but is only rotated around
                        the mesh's Z axis.
        ENormals        Encodes vertex normals. This flag is deprecated and
                        should not be used, unless you know what your doing.

Examples of legal mesh/group names:
        "box"                   Just called box
        "leaf: Billboard"       Leaf that always faces the viewer
        "leaf: BillboardZ"	Z axis rotated facing leaf

Meshes by defualt do not have any of these flags set.


Issues & Future Developement
----------------------------

- Split off the DTS SDK into it's own library and continue to refine
  it's functionality and API.

- Triangle strips. This is probably the biggest outstanding issue.
  Every mesh triangle is emitted as it's own triangle strip. This is
  very bad :( Triangles need to be stripped by material.  Support
  should be added to the DTSMesh class. 

- Detail support.  Need to add support for progressive meshes. The
  DTSMesh class should automatically produce decimated sub-details
  based off the original art.

- DSQ exporting.  DSQ files are essentially animation sequence files.
  They only contain animation sequence information and can be loaded into
  a shape at run-time.  The same DSQ can be loaded into multiple shapes
  allowing animations to be shared.

- Multiple collision meshes.  The exporter currently selects the first
  mesh named "collision" as a collision mesh.  The torque engine actually
  allows multiple collision meshes per shape and the exporter should
  export all meshes named "collision" as collision meshes.

- Billboard meshes are mesh which the Torque engine automatically
  rotates at run time to face the camera.  There needs to be some way
  of marking meshes.  This could be as simple as naming them "billboard".

- Alpha BSP (or ordered) meshes.  Alpha textured triangles need to be
  rendered in back to front order at run-time for them to render correctly.
  The dts file can contain BSP tree meshes used for this purpose.

- Better material support. There are number of material features (besides
  animation) which MS doesn't support, these include multiple UV mapping
  options, environment mapping flags, addative vs. blended alpha, etc.