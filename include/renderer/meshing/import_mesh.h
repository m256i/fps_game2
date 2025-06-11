#ifndef RENDERER_MESHING_RENDER_MESH_H_
#define RENDERER_MESHING_RENDER_MESH_H_

#include <common.h>
#include <assimp/cimport.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <containers/p_vector.h>
#include <util/dbg/alloctrack.h>

#include <math/vec3.h>
#include <math/vec2.h>

#include <math/colliders/aabb.h>
#include <math/colliders/obb.h>

/*
this is used for mesh preprocessing before transitioning into a
render_mesh that only holds the minimal needed data for rendering
the data layout of this mesh is optimized for processing not rendering
*/
typedef struct {
  u8 *raw_data;
} pre_render_mesh;

/*
we have to do this instead of using vec3s because those are oversized and have
custom alignment
*/
typedef struct __attribute__((packed)) {
  f32  position[3];
  f32  normal[3];
  vec2 texture_coords;
} import_vert;

typedef struct {
  u32  indices[3];
  vec3 normal;
} import_face;

CREATE_VECTOR_TYPE(import_vert)
CREATE_VECTOR_TYPE(import_face)

typedef struct {
  vector_import_vert verts;
  vector_import_face faces;
  char              *texture_path;
  aabb3d             aabb;
  obb3d              obb;
} import_submesh;

typedef struct {
  vector_import_vert vertices;
  u32               *indices;
  usize              index_count;
} import_mesh;

CREATE_VECTOR_TYPE(import_submesh)

static inline vec3 vec3_from_aiVec3(const struct aiVector3D *_v0) {
  return (vec3){.x = _v0->x, .y = _v0->y, .z = _v0->z};
}

static inline import_submesh process_mesh(
  const struct aiMesh *const  _mesh,
  const struct aiScene *const _scene
) {
  vector_import_vert vertex_data = vector_import_vert_create_();
  vector_import_face face_data   = vector_import_face_create_();

  vector_import_vert_reserve(&vertex_data, _mesh->mNumVertices);
  vector_import_face_reserve(&face_data, _mesh->mNumFaces);

  /*
  completely skip non triangle meshes since we triangulate and then sort by
  ptypes so we can effectively skip degenerate verts with this setup
  */
  if ((_mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
    return (import_submesh){0};

  aabb3d mesh_aabb = {0};

  mesh_aabb.min = vec3_from_aiVec3(&_mesh->mAABB.mMin);
  mesh_aabb.max = vec3_from_aiVec3(&_mesh->mAABB.mMax);

  GAME_LOGF(
    "submesh found: vertices: %u, faces: %u, indices: %u",
    _mesh->mNumVertices,
    _mesh->mNumFaces,
    _mesh->mNumFaces * 3
  );

  for (usize i = 0; i != _mesh->mNumVertices; i++) {
    import_vert v0 = {0};

    v0.position[0] = _mesh->mVertices[i].x;
    v0.position[1] = _mesh->mVertices[i].y;
    v0.position[2] = _mesh->mVertices[i].z;

    if (_mesh->mNormals != NULL) {
      v0.normal[0] = _mesh->mNormals[i].x;
      v0.normal[1] = _mesh->mNormals[i].y;
      v0.normal[2] = _mesh->mNormals[i].z;
    }

    if (_mesh->mTextureCoords[0]) {
      v0.texture_coords.x = _mesh->mTextureCoords[0][i].x;
      v0.texture_coords.y = _mesh->mTextureCoords[0][i].y;
    }
    vector_import_vert_push_back(&vertex_data, &v0);
  }

  for (usize i = 0; i != _mesh->mNumFaces; i++) {
    const struct aiFace f = _mesh->mFaces[i];
    GAME_ASSERT(f.mNumIndices == 3);

    const vec3 face_normals[3] = {
      vec3_from_aiVec3(&_mesh->mVertices[f.mIndices[0]]),
      vec3_from_aiVec3(&_mesh->mVertices[f.mIndices[1]]),
      vec3_from_aiVec3(&_mesh->mVertices[f.mIndices[2]]),
    };

    vec3 face_normal, e0, e1;

    e0          = vec3_sub(&face_normals[1], &face_normals[0]);
    e1          = vec3_sub(&face_normals[1], &face_normals[2]);
    face_normal = vec3_cross(&e0, &e1);
    vec3_normalize_inplace(&face_normal);

    vector_import_face_push_back(
      &face_data,
      &(import_face){
        .indices = {f.mIndices[0], f.mIndices[1], f.mIndices[2]},
        .normal  = face_normal,
      }
    );
  }

  const struct aiMaterial *material = _scene->mMaterials[_mesh->mMaterialIndex];

  for (usize i = 0;
       i != aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE);
       i++) {
    struct aiString texture_path;
    aiGetMaterialTexture(
      material,
      aiTextureType_DIFFUSE,
      i,
      &texture_path,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    );
    printf("material texture path: %s\n", texture_path.data);
  }

  return (import_submesh){
    .verts = vertex_data,
    .faces = face_data,
    .aabb  = mesh_aabb,
  };
}

static inline u0 process_node(
  struct aiNode         *_node,
  const struct aiScene  *_scene,
  vector_import_submesh *_meshes
) {
  for (usize i = 0; i != _node->mNumMeshes; i++) {
    struct aiMesh       *m        = _scene->mMeshes[i];
    const import_submesh new_mesh = process_mesh(m, _scene);
    vector_import_submesh_push_back(_meshes, &new_mesh);
  }

  for (usize i = 0; i != _node->mNumChildren; i++) {
    process_node(_node->mChildren[i], _scene, _meshes);
  }
}

static inline import_mesh load_mesh_from_file(const char *_path) {
  /* whole lotta flags */
  const u32 flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                    aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
                    aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
                    aiProcess_GenBoundingBoxes | aiProcess_SortByPType |
                    aiProcess_RemoveRedundantMaterials |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_ImproveCacheLocality | aiProcess_FindDegenerates |
                    aiProcess_FixInfacingNormals | aiProcess_GlobalScale;

  const struct aiScene *scene = aiImportFile(_path, flags);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    GAME_WARNF("error opening model file: '%s'", _path);
    exit(1);
  }

  vector_import_submesh import_submeshes = vector_import_submesh_create_();
  process_node(scene->mRootNode, scene, &import_submeshes);

  import_mesh out_mesh     = {0};
  usize       vertex_count = 0, index_count = 0;

  for (usize i = 0; i != vector_import_submesh_size(&import_submeshes); i++) {
    import_submesh *sm  = &import_submeshes.data[i];
    vertex_count       += sm->verts.size;
    index_count        += sm->faces.size * 3;
  }

  vector_import_vert_reserve(&out_mesh.vertices, vertex_count);
  out_mesh.indices     = TRACKED_MALLOC(index_count * sizeof(u32));
  out_mesh.index_count = index_count;

  usize base_index = 0;

  for (usize i = 0; i != vector_import_submesh_size(&import_submeshes); i++) {
    import_submesh *sm          = &import_submeshes.data[i];
    u32             base_vertex = out_mesh.vertices.size;

    vector_import_vert_insert_range(
      &out_mesh.vertices,
      out_mesh.vertices.size,
      sm->verts.data,
      sm->verts.size
    );

    for (usize j = 0; j != sm->faces.size; j++) {
      out_mesh.indices[base_index + (j * 3) + 0] =
        sm->faces.data[j].indices[0] + base_vertex;
      out_mesh.indices[base_index + (j * 3) + 1] =
        sm->faces.data[j].indices[1] + base_vertex;
      out_mesh.indices[base_index + (j * 3) + 2] =
        sm->faces.data[j].indices[2] + base_vertex;
    }
    base_index += sm->faces.size * 3;

    /* clear the vectors here */
    vector_import_vert_free(&sm->verts);
    vector_import_face_free(&sm->faces);
  }

  GAME_LOGF(
    "file %s loaded %zu vertices and %zu indeces",
    _path,
    out_mesh.vertices.size,
    out_mesh.index_count
  );

  return out_mesh;
}

#endif // RENDERER_MESHING_RENDER_MESH_H_
