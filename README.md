grapro
======

voxel fragment buffer:
    uint pos3col1
    uint col2em2
    uint em1norm3
    use (un)packUnorm4x8

octree node buffer:
    vec4 col.xyz_em.x (vec4 because of atomic add)
    vec4 em.yz_norm.xy
    float norm.z
    uint pos
    uint counter (to count how many fragments in a leaf)
    uint padding
    
1) voxelization:
    save all conservative fragments in voxel fragment buffer
2)
    a) node alloc
    b) node flag
        if leaf:
        atomicAdd color, emissive, normal, counter
    c) injectLightingInformation:
        walk over all leafs and compute radiance (walk over all lights) ("shadowmapping to octree")
