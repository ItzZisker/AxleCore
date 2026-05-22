# Core principle

A “model” is not a GPU resource.

A model is:
- hierarchy
- mesh references
- material references
- skeleton references
- metadata

while:
- meshes
- textures
- shaders
- materials

are resources.

That distinction is extremely important.

---

# Recommended architecture

Separate these concepts:

```text
ModelAsset
    immutable imported structure

ModelResource
    runtime resource references

ModelInstance
    scene object with transform/state
```

This scales very well.

---

# Recommended ownership

```text
AssetManager
 ├── MeshResource
 ├── TextureResource
 ├── MaterialResource
 ├── ShaderResource
 └── ModelResource

Scene
 └── ModelInstance
```

---

# Do we need ModelManager?

Yes.

But not because models are “special”.

Because:
- centralized ownership
- deduplication
- lifetime tracking
- async loading later
- stable handles
- hot reload later

all become easier.

You want:
- resource registries
- stable IDs
- indirect lookup

not raw pointers everywhere.

---

# Recommended handle design

Do NOT store pointers in handles.

Do NOT store GPU handles in handles.

Handles should only contain:
- index
- generation/version

Example:

```cpp
template<typename Tag>
struct Handle
{
    uint32_t index = 0;
    uint32_t generation = 0;

    bool IsValid() const
    {
        return generation != 0;
    }

    auto operator<=>(const Handle&) const = default;
};
```

Then:

```cpp
struct MeshTag {};
struct TextureTag {};
struct ModelTag {};

using MeshHandle    = Handle<MeshTag>;
using TextureHandle = Handle<TextureTag>;
using ModelHandle   = Handle<ModelTag>;
```

This is:
- type safe
- compact
- stable
- cache friendly

---

# Why generation matters

Without generation:

```text
destroy mesh
reuse slot
old handle becomes valid accidentally
```

Generation prevents stale handle reuse.

---

# Resource manager layout

Example generic pool:

```cpp
template<typename Resource, typename HandleT>
class ResourceManager
{
public:
    HandleT Create(Resource&& resource);

    void Destroy(HandleT handle);

    Resource* Get(HandleT handle);

private:
    struct Slot
    {
        Resource resource;
        uint32_t generation = 1;
        bool alive = false;
    };

    std::vector<Slot> m_Slots;
};
```

This is enough for a very serious engine.

---

# Mesh resource

Example:

```cpp
struct MeshGpuData
{
    gfx::BufferHandle vertexBuffer;
    gfx::BufferHandle indexBuffer;
};
```

```cpp
struct MeshResource
{
    MeshBounds bounds;

    std::unique_ptr<MeshGpuData> gpu;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};
```

Notice:
GPU residency is optional.

Future-proof.

---

# Texture resource

```cpp
struct TextureGpuData
{
    gfx::ImageHandle image;
};
```

```cpp
struct TextureResource
{
    uint32_t width = 0;
    uint32_t height = 0;

    std::unique_ptr<TextureGpuData> gpu;
};
```

---

# Material resource

Materials should reference resources by handles.

NOT pointers.

```cpp
struct MaterialResource
{
    ShaderHandle shader;

    TextureHandle albedo;
    TextureHandle normal;
    TextureHandle roughness;

    MaterialProps props;
};
```

This is very important for serialization and hot reload.

---

# Model resource

This is where scene hierarchy lives.

Example:

```cpp
struct ModelNode
{
    std::string name;

    glm::mat4 localTransform{1.0f};

    std::vector<uint32_t> children;

    MeshHandle mesh;
    MaterialHandle material;
};
```

Then:

```cpp
struct ModelResource
{
    std::vector<ModelNode> nodes;

    uint32_t rootNode = 0;
};
```

Notice:
the model itself contains NO GPU data.

Only references.

That’s correct architecture.

---

# Model instance

Scene object:

```cpp
struct ModelInstance
{
    ModelHandle model;

    glm::mat4 transform{1.0f};

    bool visible = true;
};
```

If animated:

```cpp
struct ModelInstance
{
    ModelHandle model;

    glm::mat4 transform;

    AnimationState animationState;
};
```

---

# Renderer flow

Renderer does:

```text
Scene
 → ModelInstance
 → ModelResource
 → MeshHandle
 → MeshResource
 → GPU data
 → RenderItem
```

Perfect separation.

---

# About ModelManager

You can either:

OPTION A:
```cpp
MeshManager
TextureManager
MaterialManager
ModelManager
```

OR:

OPTION B:
```cpp
AssetManager
```

with typed registries internally.

I recommend B.

Example:

```cpp
class AssetManager
{
public:
    MeshHandle CreateMesh(...);

    TextureHandle CreateTexture(...);

    ModelHandle CreateModel(...);

    MeshResource* GetMesh(MeshHandle);
    TextureResource* GetTexture(TextureHandle);
    ModelResource* GetModel(ModelHandle);

private:
    ResourceManager<MeshResource, MeshHandle> m_Meshes;
    ResourceManager<TextureResource, TextureHandle> m_Textures;
    ResourceManager<ModelResource, ModelHandle> m_Models;
};
```

This scales extremely well.

---

# What should NOT happen

Avoid:

```cpp
Model
 ├── actual mesh vertices
 ├── actual textures
 ├── actual GPU handles
```

That creates duplication and lifetime nightmares.

---

# About loading glTFs

Correct flow:

```text
Import glTF
    ↓
Create MeshResources
Create TextureResources
Create MaterialResources
Create ModelResource
    ↓
Destroy transient import data
```

The model references already-created resources.

---

# Future streaming compatibility

Later you can add:

```cpp
enum class ResidencyState
{
    Unloaded,
    Loading,
    Resident
};
```

inside resources.

No architecture rewrite needed.

---

# Minimal serious engine structure

This is honestly enough:

```text
Handle<T>

ResourceManager<T>

MeshResource
TextureResource
MaterialResource
ModelResource

ModelInstance

AssetManager
```

That small set of abstractions scales surprisingly far.