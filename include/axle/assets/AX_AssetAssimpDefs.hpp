#ifdef __AX_ASSETS_ASSIMP__
#pragma once

#ifndef AI_MATKEY_GLTF_ALPHACUTOFF
#define AI_MATKEY_GLTF_ALPHACUTOFF "$mat.gltf.alphaCutoff", 0, 0
#endif

#ifndef _AI_MATKEY_GLTF_SCALE_BASE
#define _AI_MATKEY_GLTF_SCALE_BASE "$tex.scale"
#endif

#ifndef AI_MATKEY_GLTF_TEXTURE_SCALE
#define AI_MATKEY_GLTF_TEXTURE_SCALE(type, N) _AI_MATKEY_GLTF_SCALE_BASE, type, N
#endif

#ifndef AI_MATKEY_GLTF_ALPHAMODE
#define AI_MATKEY_GLTF_ALPHAMODE "$mat.gltf.alphaMode", 0, 0
#endif

#endif