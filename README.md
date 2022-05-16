# COMP3015 - Project 2 - 2022

## Systems Specifics:
Operating System: Windows10 Home
OS Build: 19043.1288
Visual Studio Version: Visual Studio Community 2019 - 16.11.7

Demonstration Video: 
Git Repository: https://github.com/Bosco1020/COMP3015-CW2


## Shader Structure
The scene contains a Blinn-Phong shading for lighting, combined with a soft-edge shadow shader. There are a couple different shader files for different situations, whether textures are being used or an alpha map is needed, but they all follow the same structure with only minor changes, except for the fountain shader which is detailed later.

-Blinn-Phong is the main lighting calculation shader, using the materials information and lights information to calculate the ambient lighting, diffuse lighting across and any specular lighting on each object.

-Shadows are generated using a frustrum, moving the camera to the lights position temporarily to capture the scene image for calculating shadow positions. Soft edges are achieved through sampling nearby texels and "stretch" the shadow colour between them through a series of offset values for the width and height to apply the colour. Using a vector 3, depth is also recorded and used too determine whether a fragment is ocmpletely witin shadow, to determine whetehr to apply this function or not. 

The scene also contains a fog effect, which applies a fixed colour to each pixel and changes its intensity depending on the distance from the camera. This has been re-purposed to work vertically, to act as a pseudo water for the scene. From the minimum height, the fog starts at full strength, gettint more opaque going up until it reaches the maximum height and is invisible.

The scene also contains a particle spout, that ejects a stream if sprites to create the water fountain. The shader for which uses the to calculate where along its tragectory each individual particle is. By spacing apart their start times and applying some randomness to their start positions, a constant stream is fired for the duration, launched in the direction of initial velocity but impacted over time by gravity.


## Class Structure
The application is an ammalgimation of several systems being used for the different effects.

In order of what's run, the initScene sets up the application by generating the necessary buffers, object pools and texture bindings. These are split betweeen: the compile method that links all of the shaders, setupFBO which generates and binds all the textures and buffers including those for the shadows, and initBuffers which generates the sprite object pool and calculates the start times for each particle in the fountain. In the initScene, several variables for shaders are also set, such as those for the shadow maps, fountain variables and fog atributes.

The render method is what constantly runs, firstyly calculating the shadow map based on the lights position, then rendering the scene. 

For the shadows, 2 passes are required in the shader. to do this a sub routine is used as a parental reference for each of the methods, allowing the switching of which method is used in the script. Using this, the first pass is done by generating the scene and projection matrix from the lights position. This is then used to generate the Shadow Map, a texture map containing details as to where the shadows are located in space.

With the shadow map ready, pass 2 then renders the scene normally except each fragment is checked for whether there is any shadow at that point. If so, then shadow is applied to the pixel depending on its strength. To help create soft edges, a jitter function is used to randomly move the samples by a slight amount before sampling them in a circular pattern of nearby texels. The nested for loops in the buildJitter method allow for the creation of all the necessary sampels, the 2 dimensional space of the size requiring the nested "size" bound loops to cover each point, and then the "samples" loop to ensure the entire sample size is calculated at that point.

For the fountain effect, object pooling is used to generate all of the sprites at the start of the application. These are stored after having their positions randomised slightly, with the array holding them being triple the size to store each particles vector 3 position, recording the data in triples. For each particle, their start times are also calculated by a gradient funciton so they're all equidistent from each other between the start and end of the particle duration. This is then used to check whether the current time is greater than their start time for rendering them in the shader, multiplying their relative forces by their time to move them along their trajectories.

To reset the fountain, the time value is on a loong where after the fountain has ended it's reset to 0. Because of the shaders code functioning by using this time to calculate the sprites position and transparency, the fountain is re-started with the time value. Other values similar to this are also reset on a loop for animating other aspects of the scene.


## Installation Instructions
To open the project, download the - FILE NAME - from the Git Hub repository.
Unzip the file and then run the - FILE NAME - .exe.
