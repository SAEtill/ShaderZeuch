#define GLEW_STATIC
#include <glew.h>
#include <glfw3.h>

#include <Cg/cg.h>    /* Can't include this?  Is Cg Toolkit installed! */
#include <Cg/cgGL.h>

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>



static CGcontext   myCgContext;


static CGprofile   myCgVertexProfile;
static CGprogram   myCgVertexProgram;

static CGprofile   myCgFragmetProfile;
static CGprogram   myCgFragmetProgram;

static void checkForCgError(const char *situation)
{
	CGerror error;
	const char *string = cgGetLastErrorString(&error);

	if (error != CG_NO_ERROR) {
		printf("%s: %s: %s\n",
			"foo", situation, string);
		if (error == CG_COMPILER_ERROR) {
			printf("%s\n", cgGetLastListing(myCgContext));
		}
		char dummy[30];
		scanf_s("%s", dummy);
		exit(1);
	}
}

float randf(float max) {
	int irand = rand() % 1000;
	return irand * max / 1000;
}


void cross(float* a, float *b, float* K){
#define Y [1]
#define X [0]
#define Z [2]

	K X = a Y * b Z - a Z * b Y;
	K Y = a X * b Z - a Z * b X;
	K Z = a X * b Y - a Y * b X;
#undef X
#undef Y
#undef Z
}


void normalize(float * a, float * res){
	float len = 0;
	for (int k = 0; k < 3; k++) len += a[k] * a[k];
	len = sqrtf(len);
	for (int k = 0; k < 3; k++) res[k] = a[k] / len;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(500, 500, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glewInit();//getprocaddress to make opengl work wih a higher version than 1.2 - easier way to get function pointer

	myCgContext = cgCreateContext();
	checkForCgError("creating context");

	myCgVertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	checkForCgError("selecting vertex profile");

	myCgFragmetProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
	checkForCgError("selecting vertex profile");

	

	char * prog_src =
		"struct C2E1v_Output {"
		"	float4 position : POSITION;"
		"	float3 color : COLOR;"
		"   float3 light_pos : TEXCOORD1; "
		"   float3 t_pos     : TEXCOORD0; "
		"   float3 normal    : TEXCOORD2;    "
		"};"
		""
		"C2E1v_Output C2E1v_green(float3 position : POSITION, float3 norm : NORMAL, float3 light_pos :TEXCOORD,   float3 col : COLOR)"
		"{"
		"	C2E1v_Output OUT;                                  "
		"   OUT.normal    = norm ;                         "
		"	OUT.position  = float4(position, 1);           "
		"	OUT.color     =  col;                      "
		"   OUT.t_pos     =  position;                 "
		"   OUT.light_pos = light_pos;                 "
		"	return OUT;"
		"}";

	/*
	Cin * mat_col + Ambient * mat_col;
	float3 Cin = col *  i  * 1/(light_dist * light_dist );
	float i = max(dot(norm, light_direction), float3(0, 0, 0));

	float3 light_direction = light_pos - position;     
	float light_dist = length(light_direction);                                                   "
	light_direction = normalize ( light_direction); 
	float3 mat_col = float3(0.2, 0.7, 0.3);                
	*/

	char * frag_src = ""
		"float3 main(float3 light_pos:TEXCOORD1 , float3 t_pos:TEXCOORD0, float3 col : COLOR  , float3 normal : TEXCOORD2  ) :COLOR "
		"{"
		"     float3 light_dir   = light_pos - t_pos;                                       "
		"     float light_dist   = length(light_dir );                                       "
		"     light_dir = normalize(light_dir);                                              "
		"     float dist_fac = 1 / (light_dist * light_dist);                                     "
		"     float angular_fac  = max(0 , dot(light_dir,normal));                           "
		"     /* return float3(1,1,1) -  float3(light_dist , light_dist , light_dist ) *0.5 * angular_fac ;*/ "
		"     return angular_fac * dist_fac * float3(1,1,0)+  float3 ( 0, 0 , t_pos.z) ;       "
		"}"
		"";

	myCgVertexProgram = cgCreateProgram(
		myCgContext,              /* Cg runtime context */
		CG_SOURCE,
		prog_src,
		myCgVertexProfile,
		"C2E1v_green",
		NULL

		);
	checkForCgError("creating vertex program from file");

	myCgFragmetProgram = cgCreateProgram(
		myCgContext,              /* Cg runtime context */
		CG_SOURCE,
		frag_src,
		myCgFragmetProfile,
		"main",
		NULL

		);
	checkForCgError("creating fragment program from file");


	
	cgGLLoadProgram(myCgVertexProgram);
	checkForCgError("loading vertex program");
	cgGLLoadProgram(myCgFragmetProgram);
	checkForCgError("loading fragment program");

	float angle = 0.0f;
	float pih = (float)acos(0);//pi halbe


	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#define no_quads_x  50
#define no_quads_y  50

	float grid[no_quads_y+1][no_quads_x+1][3];

	float qw = 2.0f / no_quads_x;
	float qh = 2.0f / no_quads_y;

	for (int i = 0; i < no_quads_x+1; i++) {
		for (int j = 0; j < no_quads_y+1; j++) {
			grid[j][i][0] = -1 + i * qw;
			grid[j][i][1] = -1 + j * qh;
			grid[j][i][2] = 0.2 +randf(1.0 / 100 );
		}
	}
	float grid_normal[no_quads_y][no_quads_x][2][3];

	for (int i = 0; i < no_quads_y; i++)
		for (int j = 0; j < no_quads_x; j++){
		float v1[3];
		float v2[3];
		for (int k = 0; k < 3; k++) v1[k] = grid[i][j+1][k] - grid[i][j][k];
		for (int k = 0; k < 3; k++) v2[k] = grid[i+1][j][k] - grid[i][j][k];
		float v3[3];
		float v4[3];
		for (int k = 0; k < 3; k++) v3[k] = grid[i +1 ][j + 1][k] - grid[i][j+1][k];
		for (int k = 0; k < 3; k++) v4[k] = grid[i + 1][j    ][k] - grid[i][j+1][k];



		cross(v2, v1, grid_normal[i][j][0]);
		cross(v4, v3, grid_normal[i][j][1]);
		normalize(grid_normal[i][j][0], grid_normal[i][j][0]);
		normalize(grid_normal[i][j][1], grid_normal[i][j][1]);
		printf(" %f %f %f\n", grid_normal[i][j][1][0], grid_normal[i][j][1][1], grid_normal[i][j][1][2]);
		}
			

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		cgGLBindProgram(myCgVertexProgram);
		checkForCgError("binding vertex program");
		cgGLEnableProfile(myCgVertexProfile);
		checkForCgError("enabling vertex profile");


		cgGLBindProgram(myCgFragmetProgram);
		checkForCgError("binding fragment program");
		cgGLEnableProfile(myCgFragmetProfile);
		checkForCgError("enabling fragment profile");

		angle += 0.005f;
		
		glColor3f(1, 1, 1);
		glTexCoord3f(0.5*sin(angle ) , 0.5*cos (angle),-1);
	
		
		for (int i = 0; i < no_quads_x; i++) {
			for (int j = 0; j < no_quads_y; j++) {
				glBegin(GL_TRIANGLES);
				glNormal3fv(grid_normal[j][i]);

				/*
				    [1,0]    [1,1]
				    (3)      (2) 
				    
				    
					[0,0]    [0,1]
					(0)      (1)
					
				
				*/
				
				// tri 1 
				glNormal3fv(grid_normal[j][i][0]);

				glVertex3f( grid[j  ][i  ][0] , grid[j][i][1] , grid[j][i][2]);  //  0
				glVertex3fv(grid[j  ][i+1]);                                     //  1
				glVertex3fv(grid[j+1][i  ]);                                     //  3

				// tri 2
				glNormal3fv(grid_normal[j][i][1]);
				glVertex3fv(grid[j  ][i+1]);                                  // 1 
				glVertex3fv(grid[j+1][i+1]);                                  // 2 
				glVertex3fv(grid[j+1][i  ]);                              // 3
				glEnd();
			}
		}
		assert(glGetError() == GL_NO_ERROR);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}