#pragma once

// Bone icon mesh — the in-viewport visual for the WWSkin Space Warp Object.
// Vertex/face data is taken verbatim from EA's boneicon.cpp (Westwood's
// hand-modelled bone shape, 184 verts / 366 faces). Used by SkinWSMObjectClass::
// BuildMesh to populate its SimpleWSMObject base mesh.

namespace W3D::MaxTools
{
	struct BoneIconVertex { float X, Y, Z; };
	struct BoneIconFace   { int   V0, V1, V2; };

	extern const int               NumBoneIconVerts;
	extern const int               NumBoneIconFaces;
	extern const BoneIconVertex    BoneIconVerts[];
	extern const BoneIconFace      BoneIconFaces[];
}
