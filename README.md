# CppDirectXRayTracing
DirectX Raytracing Tutorials in C++

The main references of this tutorial are [CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing) and [Nvidia tutorial](https://github.com/NVIDIAGameWorks/DxrTutorials). If not mentioned in the code, the tutorials from 01-07 are reconstructed from [Nvidia tutorial](https://github.com/NVIDIAGameWorks/DxrTutorials), the 15-19 are ported from [CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing). Part of the shader code in GI is modified from [SIGGRAPH 2018 Course](http://intro-to-dxr.cwyman.org/). All of those are awesome and useful resources to read. 
 
The tutorial is served as the reconstruction and extension work of the Nvidia tutorial. The refactoring of the code is aiming at a eaiser understanding of DXR, and easier for the learners to extend. Unlike Falcor, the code doesn't has abstraction on the CPU host side.  

The GI solution is packaged in [release](https://github.com/qingqhua/CppDirectXRayTracing/releases) for win64 system.

## Environment 
Windows SDK 10.0.18362.0  
Visual Studio 2019  
GTX 1070 GPU Card (You can find if you graphics card supports DXR [here](https://linuxhint.com/nvidia-cards-support-ray-tracing/))  

## Tutorials
The image shot of each sub-project, the projects can easily be run by clicking *CppDirectXRayTracing.sln*. Note the 7-14 is blank just because the tutorials diverged from the original nv turotials and I was too lazy to rename them.

### Tutorial 1-6
Basic setting of the DXR.
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial6.PNG?raw=true)

### Tutorial 7
Shader for ray-traced triangle
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial7.PNG?raw=true)

### Tutorial 15
Pass vertex buffer and index bufer to shader instead of hand-made triangles.  
The geometry construction is under *Primitives*.
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial15.PNG?raw=true)

### Tutorial 16
Simple Phong lighting. Most of the changes are in the shader.
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial16.PNG?raw=true)

### Tutorial 17
How to pass multiply geometry in to shader. This one combined the geometry into one vertex and index buffer, which is not practical, but doable.  
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial17.PNG?raw=true)  

### Tutorial 17-2
The second way for passing multiple geometry.  
The spheres are sent by index buffer and vertex buffer, the normals of the plane are tricky, specified in shader.   
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial17-2.PNG?raw=true)

### Tutorial 18
How to cast shadow ray and third way for passing multiple geometry. This is the same approach as Nvidia tutorial.  
Bind primitive to different BLAS, each of the primitive has its own closest shader. Also instances of the spheres are binded to TLAS.  
The aliasing of the shadow is potentially a bug in this way of sending geometry.  
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial18.PNG?raw=true)

### Tutorial 19
Reflections with Phong lighting. Shoot the reflection rays. Also see shaders in [CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing/tree/master/18-Reflection/Data)  
From now on, we stick to the second way of sending the geometry, which works best.  
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial19.PNG?raw=true)

### Tutorial 20
Set scene constant buffer and primitive constant buffer. This is useful for setting different materials.
![](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial20.PNG?raw=true)

### Tutorial 21
How to do full global illumination in DXR. This tutorial implemented naive recursive ray tracer with Lambertian and GGX model. This sub-project is packaged as an win64-exe in [release](https://github.com/qingqhua/CppDirectXRayTracing/releases)   
Use keyboard number 1 to switch between Lambertian GI and AO with direct lighting.  
Use keyboard number 2 to open GGX shading.  
Use keyboard number 3 to open dynamic lighting.  
#### Lambertian GI:
![Lambertian GI](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial21-lambdertian.PNG?raw=true)  
#### GGX GI:
![GGX GI](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial21-ggx.PNG?raw=true)

#### AO with only direct light:
![AO with only direct light](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial21-ao.PNG)

##### Dynamic Lighting:
![Dynamic Lighting](https://github.com/qingqhua/CppDirectXRayTracing/blob/main/images/tutorial21-dynamiclight.PNG?raw=true)

## Contribution
You are very welcomed to submit issues, extend the tutorial (e.g. better GI solution with less noise, techniques in Ray Tracing Gem), code quality improvements, code comment improvements, etc.

## Resources
The [Nvidia tutorial](https://github.com/NVIDIAGameWorks/DxrTutorials) has a code walk through document, make sure to check it.  
[SIGGRAPH 2018 Course](http://intro-to-dxr.cwyman.org/) Good explanation of how to integrate the shader for global illumination.  
[Ray Tracing in One Weekend](https://raytracing.github.io/) The basic theory of GI. The first among the series introduced the basics of ray tracing, the third among the series introduces the Monte Carlo strategy.
