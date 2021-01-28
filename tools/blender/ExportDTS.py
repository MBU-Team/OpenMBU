#
# Torque Shader Engine DTS exporter for blender
#
import Blender
import string
#from Blender import NMesh,Object
from math import *

VERSION = "0.1"

##############################
# Vector class
##############################
class Vector:
	members = [0, 0, 0]
	def __init__(self, x, y, z):
		self.members = [float(x), float(y), float(z)]
	def __getitem__(self, key):
		return(self.members[key])
	def __setitem__(self, key, value):
		self.members[key] = value
	def __add__(self, other):
		# iterate through the members
		for i in range(len(self.members)):
			# add them together
			self[i] = float(self[i]) + float(other[i])
		return(self)
	def __sub__(self, other):
		# iterate through the members
		for i in range(len(self.members)):
			# subtract them
			self[i] = float(self[i]) - float(other[i])
		return(self)
	def __mul__(self, other):
		# iterate through the members
		for i in range(len(self.members)):
			# multiply by the val stored in other
			self[i] *= float(other)
		return(self)
	def __div__(self, other):
		# iterate through the members
		for i in range(len(self.members)):
			# divide by the val stored in other
			self[i] /= float(other)
		return(self)
	#def normalize(self):
##############################
# end of Vector class
##############################
##############################
# Node class - DTS tree node
##############################
class Node:
	name = 0				# index of its name in the DTS string table
	parent = -1			# number of the parent node; -1 if root
	firstObject = -1	# deprecated; set to -1
	child = -1			# deprecated; set to -1
	sibling = -1		# deprecated; set to -1
##############################
# end of Node class
##############################
##############################
# Object class - DTS object
##############################
class Object:
	name = 0				# index of its name in the DTS string table
	numMeshes = 0		# number of meshes (only one for detail level)
	firstMesh = 0		# number of the first mesh (meshes must be consecutive)
	node = 0				# number of the node where the object is stored
	sibling = -1		# deprecated; set to -1
	firstDecal = -1	# deprecated; set to -1
##############################
# end of Object class
##############################
##############################
# Material class - DTS material
##############################
class Material:
	name = 0				# texture name; materials don't use the DTS string table
	flags = 0			# boolean properties
	reflectance = 0	# number of reflectance map?
	bump = 0				# number of bump map? or -1 if none
	detail = 0			# number of detail map? or -1 if none
	detailScale = 0	# ?
	reflection = 0		# ?
	# material flags
	SWrap             = 0x00000001
	TWrap             = 0x00000002
	Translucent       = 0x00000004
	Additive          = 0x00000008
	Subtractive       = 0x00000010
	SelfIlluminating  = 0x00000020
	NeverEnvMap       = 0x00000040
	NoMipMap          = 0x00000080
	MipMapZeroBorder  = 0x00000100
	IFLMaterial       = 0x00000000
	IFLFrame          = 0x10000000
	DetailMap         = 0x20000000
	BumpMap           = 0x40000000
	ReflectanceMap    = 0x80000000
	AuxiliaryMask     = 0xF0000000
##############################
# end of Material class
##############################
##############################
# IFLMaterial class - DTS animated material
##############################
class IFLMaterial:
	name = 0
	slot = 0
	firstFrame = 0
	time = 0
	numFrames = 0
##############################
# end of IFLMaterial class
##############################
##############################
# DetailLevel class - DTS detail level
##############################
class DetailLevel:
	name = 0				# index of the name in the DTS string table
	subshape = 0		# number of the subshape it belongs to
	objectDetail = 0	# number of object mesh to draw for each object
	size = 0				# minimum pixel size
	avgError = -1		# ?
	maxError = -1		# ?
	polyCount = 0		# polygon count of the meshes to draw
##############################
# end of DetailLevel class
##############################
##############################
# Subshape class - DTS subshape (defines a range of nodes and objects)
##############################
class Subshape:
	firstNode = 0
	firstObject = 0
	firstDecal = 0
	numNodes = 0
	numObjects = 0
	numDecals = 0
	firstTranslucent = 0	# not used?
##############################
# end of Subshape class
##############################
##############################
# ObjectState class - related to animated materials?
##############################
class ObjectState:
	vis = 0
	frame = 0
	matFrame = 0
##############################
# end of ObjectState class
##############################
##############################
# Trigger class
##############################
class Trigger:
	state = 0
	pos = 0
##############################
# end of Trigger class
##############################
##############################
# Sequence class
##############################
	nameIndex = 0
	flags = 0
	numKeyFrames = 0
	duration = 0
	priority = 0
	firstGroundFrame = 0
	numGroundFrames = 0
	baseRotation = 0
	baseTranslation = 0
	baseScale = 0
	baseObjectState = 0
	baseDecalState = 0
	firstTrigger = 0
	numTriggers = 0
	toolBegin = 0
	matters = 0
	# flags
	UniformScale    = 0x0001
	AlignedScale    = 0x0002
	ArbitraryScale  = 0x0004
	Blend           = 0x0008
	Cyclic          = 0x0010
	MakePath        = 0x0020
	IFLInit         = 0x0040
	HasTranslucency = 0x0080
##############################
# end of Sequence class
##############################
##############################
# Matters class
##############################
class Matters:
	rotation = 0
	translation = 0
	scale = 0
	decal = 0
	ifl = 0
	vis = 0
	frame = 0
	matframe = 0
##############################
# end of Matters class
##############################
##############################
# Box class
##############################
class Box:
	min = Vector
	max = Vector
##############################
# end of Box class
##############################
##############################
# Primitive class
##############################
class Primitive:
	firstElement = 0
	numElements = 0
	type = 0
	# types
	Triangles    = 0x00000000
	Strip        = 0x40000000
	Fan          = 0x80000000		# may not be supported in the engine?
	TypeMask     = 0xC0000000
	Indexed      = 0x20000000		# not supported in the engine?
	NoMaterial   = 0x10000000
	MaterialMask = 0x0FFFFFFF
##############################
# end of Primitive class
##############################
##############################
# Primitive class
##############################
class Mesh:
	# members
	type = 0
	numFrames = 0
	matFRames = 0
	parent = 0

	verts = []
	tverts = []
	normals = []
	enormals = []
	primitives = []
	indices = []
	mindices = []
	### BOX ###
	bounds = Box
	### BOX ###
	center = Vector(0, 0, 0)
	radius = 0
	vertsPerFrame = 0
	flags = 0
	# data used by skin meshes
	vindex = 0
	vbone = 0
	vweight = 0
	nodeIndex = 0
	nodeTransform = 0

	# types
	T_Standard  = 0
	T_Skin      = 1
	T_Decal     = 2
	T_Sorted    = 3
	T_Null      = 4
	# flags
	Billboard      = 0x80000000
	HasDetail      = 0x40000000
	BillboardZ     = 0x20000000
	EncodedNormals = 0x10000000

	def __init__(self, t):
		self.bounds.min = Vector(0, 0, 0)
		self.bounds.max = Vector(0, 0, 0)
		self.center = Vector(0, 0, 0)
		self.radius = float(0.0)
		self.numFrames = 1
		self.matFrames = 1
		self.vertsPerFrame = 0
		self.parent = -1
		self.flags = 0
		self.type = t
	def getType(self):
		return(type)
	def setType(self, t):
		type = t
	def setFlag(self, f):
		flags |= f
	def getPolyCount(self):
		count = 0
		for p in range(len(primitives)):
			if (primitives[p].type & Primitive.Stripe):
				count += primitives[p].numElements - 2
			else:
				count += primitives[p].numElements / 3
		return(count)
	def getRadius(self):
		return(radius)
	def getCenter(self):
		return(center)
	def getBounds(self):
		return(bounds)
	def setMaterial(self, n):
		for p in range(len(primitives)):
			p.type = (p.type & ~Primitive.MaterialMask) | (n & Primitive.MaterialMask)
	def getVertexBone(self, node):
		b = 0
		for b in range(len(nodeIndex)):
			if (nodeIndex[b] == node):
				return(b)
	def getNodeIndexCount(self):
		return(len(nodeIndex))
	def getNodeIndex(self, node):
		if node >= 0:
			if node < len((nodeIndex)):
				return(nodeIndex[node])
		return(-1)
	#def setNodeTransform(self, node, t, q):
	def setCenter(self, c):
		center = c
	def setBounds(self, b):
		bounds = b
	def setRadius(self, r):
		radius = r
	def setFrames(self, n):
		self.numFrames = n
		self.vertsPerFrame = len(self.verts)/n
	def setParent(self, n):
		parent = n
	def calculateBounds(self):
		self.bounds.max = Vector(-10e30, -10e30, -10e30)
		self.bounds.min = Vector(10e30, 10e30, 10e30)
		for vertex in self.verts:
			if vertex[0] < self.bounds.min[0]:
				self.bounds.min[0] = vertex[0]
			if vertex[1] < self.bounds.min[1]:
				self.bounds.min[1] = vertex[1]
			if vertex[2] < self.bounds.min[2]:
				self.bounds.min[2] = vertex[2]
			if vertex[0] > self.bounds.max[0]:
				self.bounds.max[0] = vertex[0]
			if vertex[1] > self.bounds.max[1]:
				self.bounds.max[1] = vertex[1]
			if vertex[2] > self.bounds.max[2]:
				self.bounds.max[2] = vertex[2]
	def calculateCenter(self):
		for v in range(len(self.bounds.max.members)):
			self.center[v] = ((self.bounds.min.members[v] - self.bounds.max.members[v])/2) + self.bounds.max.members[v]
	def calculateRadius(self):
		for vertex in self.verts:
			tV = vertex - self.center
			result = 0
			for n in range(len(tV.members)):
				result += tV.members[n] * tV.members[n]
			distance = sqrt(result)
			if distance > self.radius:
				self.radius = distance
	def encodeNormal(self, p):
		bestIndex = 0
	#def save(self):
##############################
# end of Mesh class
##############################
##############################
# Shape class
##############################
class Shape:
	nodes = 0
	objects = 0
	decals = 0
	subshapes = 0
	IFLmaterials = 0
	materials = 0
	nodeDefRotations = 0
	nodeDefTranslations = 0

	#def save(self):
	#def read(self):
	#def getBounds(self):
	#def getRadius(self):
	#def getTubeRadius(self):
	#def addName(self, s):
	#def calculateBounds(self):
	#def calculateRadius(self):
	#def calculateTubeRadius(self):
	#def calculateCenter(self):
	#def setSmallestSize(self, i):
	#def setCenter(self, p):
	#def getNodeWorldPosRot(self, n, trans, rot):
	#def write(self):
##############################
# end of Shape class
##############################

# functions
# strips the path off of a filepath specified.
def basename(filepath):
	if "\\" in filepath:
		words = string.split(filepath, "\\")
	else:
		words = string.split(filepath, "/")
	words = string.split(words[-1], ".")
	return string.join(words[:-1], ".")

def process_objects(objnames):
	# objects
	dtsobjs = []
	numdtsobjs = 0
	print "Objects:"
	for objname in objnames:
		if not type(objname.data) == Blender.Types.NMeshType:
			continue
		print "--> ", objname.data.name
		print "location = ", objname.getLocation()
		meshobj = Blender.NMesh.GetRaw(objname.data.name)
		dtsobj = Mesh(Mesh.T_Standard)
		dtsobj.numFrames = 1
		dtsobj.matFrames = 1
		dtsobj.verts = []
		dtsobj.tverts = []
		dtsobj.normals = []
		dtsobj.enormals = []
		# get verts, uv coords, normals, and enormals
		for v in range(len(meshobj.verts)):
			###print "v = ", v, "x = ", meshobj.verts[v].co[0], ", y = ", meshobj.verts[v].co[1], ", z = ", meshobj.verts[v].co[2]
			# verts
			dtsobj.verts.append(Vector(meshobj.verts[v].co[0], meshobj.verts[v].co[1], meshobj.verts[v].co[2]))

			# uv coords
			dtsobj.tverts.append(meshobj.verts[v].uvco[0])
			dtsobj.tverts.append(meshobj.verts[v].uvco[1])

			# normals
			dtsobj.normals.append(meshobj.verts[v].no[0])
			dtsobj.normals.append(meshobj.verts[v].no[1])
			dtsobj.normals.append(meshobj.verts[v].no[2])

			# enormals
			dtsobj.enormals.append(meshobj.verts[v].no[0])
			dtsobj.enormals.append(meshobj.verts[v].no[1])
			dtsobj.enormals.append(meshobj.verts[v].no[2])
		for f in meshobj.faces:
			print "vIx = ", meshobj.verts.index(f.v[0]), "vIy = ", meshobj.verts.index(f.v[1]), "vIz = ", meshobj.verts.index(f.v[2])
			dtsobj.indices.append(meshobj.verts.index(f.v[2]))
			dtsobj.indices.append(meshobj.verts.index(f.v[1]))
			dtsobj.indices.append(meshobj.verts.index(f.v[0]))

		# create our primitive
		p = Primitive
		p.firstElement = len(dtsobj.indices)
		p.numElements = 3
		p.type = Primitive.Strip | Primitive.Indexed
		# no materials for now
		p.type |= Primitive.NoMaterial

		dtsobj.setFrames(1)
		dtsobj.setParent(-1)
		dtsobj.calculateBounds()
		dtsobj.calculateCenter()
		dtsobj.calculateRadius()
		dtsobj.vertsPerFrame = len(dtsobj.verts)

		dtsobj.primitives.append(p)
		dtsobjs.insert(numdtsobjs, dtsobj)
		numdtsobjs += 1

##############################
# main logic
##############################
if __name__ == "__main__":
	print "DTS Exporter running..."

	# filename to write out
	filename = basename(Blender.Get("filename")) + ".dts"
	print "Writing file: \"%s\"" % filename

	# get all of the objects 
	objs = Blender.Object.get()
	# count the objects
	numdtsobjs = 0
	for objname in objs:
		if not type(objname.data) == Blender.Types.NMeshType:
			continue
		numdtsobjs += 1
	if numdtsobjs <= 0:
		print "Nothing to export"
		exit

   # process our objects
	process_objects(objs)

# fun times
	t = Vector(1, 2, 3)
	s = Vector(3, 3, 4)
	v = s / 2
	print "x = ", v[0], ", y = ", v[1], ", z = ", v[2]
	print "Primitive.Strip = 0x%x" % Primitive.Strip
# end of the fun times

	print "DTS Exporter complete."
