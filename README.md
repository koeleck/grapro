grapro
======

voxel fragment buffer:  
&nbsp;&nbsp;&nbsp;&nbsp;uint pos3col1  
&nbsp;&nbsp;&nbsp;&nbsp;uint col2em2  
&nbsp;&nbsp;&nbsp;&nbsp;uint em1norm3  
&nbsp;&nbsp;&nbsp;&nbsp;use (un)packUnorm4x8  

octree node buffer:  
&nbsp;&nbsp;&nbsp;&nbsp;vec4 col.xyz_em.x (vec4 because of atomic add)  
&nbsp;&nbsp;&nbsp;&nbsp;vec4 em.yz_norm.xy  
&nbsp;&nbsp;&nbsp;&nbsp;float norm.z  
&nbsp;&nbsp;&nbsp;&nbsp;uint pos  
&nbsp;&nbsp;&nbsp;&nbsp;uint counter (to count how many fragments in a leaf)  
&nbsp;&nbsp;&nbsp;&nbsp;uint padding  
    
1) voxelization:  
&nbsp;&nbsp;&nbsp;&nbsp;save all conservative fragments in voxel fragment buffer  
2)  
&nbsp;&nbsp;&nbsp;&nbsp;a) node alloc  
&nbsp;&nbsp;&nbsp;&nbsp;b) node flag  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if leaf:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;atomicAdd color, emissive, normal, counter  
&nbsp;&nbsp;&nbsp;&nbsp;c) injectLightingInformation:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;walk over all leafs and compute radiance (walk over all lights) ("shadowmapping to octree"):
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;27 mal: speicher in 3d textur in 3x3 texels
