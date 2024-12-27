Assignment #3: Ray tracing

FULL NAME: Jiale Wang


MANDATORY FEATURES
------------------

<Under "Status" please indicate whether it has been implemented and is
functioning correctly.  If not, please explain the current status.>

Feature:                                 Status: finish? (yes/no)
-------------------------------------    -------------------------
1) Ray tracing triangles                  yes

2) Ray tracing sphere                     yes

3) Triangle Phong Shading                 yes

4) Sphere Phong Shading                   yes

5) Shadows rays                           yes

6) Still images                           yes
   
7) Extra Credit (up to 20 points)
   I did Antialiasing and Recursive reflection. For Antialiasing, I used multy-raycast (in my case it's 4) from every pixels and 
   then decide the final color based on the average of colors get by these rays. For Recursive reflection, I modified the basic ray 
   tracing code, figured out how to recurse the ray back. I spent a lot of time debugging why sometimes my sphere is on top of each 
   other when doing recursive reflection. It turned out to be I tested sphere interesection first but forgot to set the sphere to null
   when there is a closer triangle. But I have a problem when saving images in recursive reflection mode. It works well in my code but when
   saving into images, Some reflections just turned really wired. 
   (the reflection images in the images folder are screenshots, not still images saved by program; the normal images are saved by program)

   I also added a mode Feature in the 4th input argument: if you set it to 0, 
   it's normal mode. if you set it to 1, it will draw using recursive reflection mode. like this: 
   ./hw3 SIGGRAPH.scene images/SIGGRAPH_normal.JPEG 0
   ./hw3 SIGGRAPH.scene images/SIGGRAPH_reflection.JPEG 1