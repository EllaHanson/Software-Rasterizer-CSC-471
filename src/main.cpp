/* Release code for program 1 CPE 471 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>

#include "tiny_obj_loader.h"
#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;

int g_width, g_height;
int scale, offX, offY;

float W2PX(float input) {
   // scale
   // changes values from [-1, 1] to [0, 1] scale
   float ret = (input + 1) / 2;

   // translate
   // change to scale of total pixel width [0, w-1]
   ret = (ret * (scale - 1)) + offX;

   return ret;
}

float W2PY(float input) {
   // scale
   // changes values from [-1, 1] to [0, 1] scale
   float ret = (input + 1) / 2;

   // translate
   // change to scale of total pixel width [0, w-1]
   ret = (ret * (scale - 1)) + offY;

   return ret;
}


/*
   Helper function you will want all quarter
   Given a vector of shapes which has already been read from an obj file
   resize all vertices to the range [-1, 1]
 */
void resize_obj(std::vector<tinyobj::shape_t> &shapes){
   float minX, minY, minZ;
   float maxX, maxY, maxZ;
   float scaleX, scaleY, scaleZ;
   float shiftX, shiftY, shiftZ;
   float epsilon = 0.001;

   minX = minY = minZ = 1.1754E+38F;
   maxX = maxY = maxZ = -1.1754E+38F;

   //Go through all vertices to determine min and max of each dimension
   for (size_t i = 0; i < shapes.size(); i++) {
      for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
         if(shapes[i].mesh.positions[3*v+0] < minX) minX = shapes[i].mesh.positions[3*v+0];
         if(shapes[i].mesh.positions[3*v+0] > maxX) maxX = shapes[i].mesh.positions[3*v+0];

         if(shapes[i].mesh.positions[3*v+1] < minY) minY = shapes[i].mesh.positions[3*v+1];
         if(shapes[i].mesh.positions[3*v+1] > maxY) maxY = shapes[i].mesh.positions[3*v+1];

         if(shapes[i].mesh.positions[3*v+2] < minZ) minZ = shapes[i].mesh.positions[3*v+2];
         if(shapes[i].mesh.positions[3*v+2] > maxZ) maxZ = shapes[i].mesh.positions[3*v+2];
      }
   }

	//From min and max compute necessary scale and shift for each dimension
   float maxExtent, xExtent, yExtent, zExtent;
   xExtent = maxX-minX;
   yExtent = maxY-minY;
   zExtent = maxZ-minZ;
   if (xExtent >= yExtent && xExtent >= zExtent) {
      maxExtent = xExtent;
   }
   if (yExtent >= xExtent && yExtent >= zExtent) {
      maxExtent = yExtent;
   }
   if (zExtent >= xExtent && zExtent >= yExtent) {
      maxExtent = zExtent;
   }
   scaleX = 2.0 /maxExtent;
   shiftX = minX + (xExtent/ 2.0);
   scaleY = 2.0 / maxExtent;
   shiftY = minY + (yExtent / 2.0);
   scaleZ = 2.0/ maxExtent;
   shiftZ = minZ + (zExtent)/2.0;

   //Go through all verticies shift and scale them
   for (size_t i = 0; i < shapes.size(); i++) {
      for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
         shapes[i].mesh.positions[3*v+0] = (shapes[i].mesh.positions[3*v+0] - shiftX) * scaleX;
         assert(shapes[i].mesh.positions[3*v+0] >= -1.0 - epsilon);
         assert(shapes[i].mesh.positions[3*v+0] <= 1.0 + epsilon);
         shapes[i].mesh.positions[3*v+1] = (shapes[i].mesh.positions[3*v+1] - shiftY) * scaleY;
         assert(shapes[i].mesh.positions[3*v+1] >= -1.0 - epsilon);
         assert(shapes[i].mesh.positions[3*v+1] <= 1.0 + epsilon);
         shapes[i].mesh.positions[3*v+2] = (shapes[i].mesh.positions[3*v+2] - shiftZ) * scaleZ;
         assert(shapes[i].mesh.positions[3*v+2] >= -1.0 - epsilon);
         assert(shapes[i].mesh.positions[3*v+2] <= 1.0 + epsilon);
      }
   }
}


int main(int argc, char **argv)
{
	if(argc < 6) {
      cout << "Usage: raster meshfile imagefile width height colormode" << endl;
      return 0;
   }
	// OBJ filename
	string meshName(argv[1]);
	string imgName(argv[2]);
   g_width = stoi(argv[3]);
   g_height = stoi(argv[4]);
   int color_mode = stoi(argv[5]);
   vector<float> depth_arr (g_width * g_height, -1.0f);

   if (color_mode != 0 && color_mode != 1) {
      cout << "Color mode option not supported." << endl;
      return 0;
   }

   scale = min(g_width, g_height);
   offX = g_width / 2;
   offY = g_height / 2;
   if (g_width < g_height) {
      scale = g_width;
      offX = 0;
      offY = (g_height - scale) / 2;
   }
   else {
      scale = g_height;
      offX = (g_width - scale) / 2;
      offY = 0;
   }

   //create an image
	auto image = make_shared<Image>(g_width, g_height);

	// triangle buffer
	vector<unsigned int> triBuf;
	// position buffer
   // [x0, y0, z0, x1, y1, z1, x2, y2, x2]
	vector<float> posBuf;
	// Some obj files contain material information.
	// We'll ignore them for this assignment.
	vector<tinyobj::shape_t> shapes; // geometry
	vector<tinyobj::material_t> objMaterials; // material
	string errStr;
	
   bool rc = tinyobj::LoadObj(shapes, objMaterials, errStr, meshName.c_str());
	/* error checking on read */
	if(!rc) {
		cerr << errStr << endl;
	} else {
 		//keep this code to resize your object to be within -1 -> 1
   	resize_obj(shapes); 
		posBuf = shapes[0].mesh.positions;
		triBuf = shapes[0].mesh.indices;
	}

   int num_tri = triBuf.size() / 3;
   for (int i = 0; i < num_tri; i++) {
      // indeces of da triangle
      int i0 = triBuf[3*i];
      int i1 = triBuf[3*i+1];
      int i2 = triBuf[3*i+2];

      // pixel coords for vertices of triangle
      float a_x = W2PX(posBuf[3*i0]);
      float a_y = W2PY(posBuf[3*i0 + 1]);
      float a_z = posBuf[3*i0 + 2];

      float b_x = W2PX(posBuf[3*i1]);
      float b_y = W2PY(posBuf[3*i1 + 1]);
      float b_z = posBuf[3*i1 + 2];

      float c_x = W2PX(posBuf[3*i2]);
      float c_y = W2PY(posBuf[3*i2 + 1]);
      float c_z = posBuf[3*i2 + 2];

      // find bounding box
      int minX = min(a_x, min(b_x, c_x));
      int maxX = max(a_x, max(b_x, c_x));
      int minY = min(a_y, min(b_y, c_y));
      int maxY = max(a_y, max(b_y, c_y));

      if (minX < 0) minX = 0;
      if (minY < 0) minY = 0;
      if (maxX >= g_width) maxX = g_width - 1;
      if (maxY >= g_height) maxY = g_height - 1;

      // iterate through all pixels in the bounding box
      for (int x = minX; x <= maxX; x++) {
         for (int y = minY; y <= maxY; y++) {
            float area = (((b_x - a_x) * (c_y - a_y)) - ((c_x - a_x) * (b_y - a_y)));
				float b_area = (((a_x - c_x) * (y - c_y)) - ((x - c_x) * (a_y - c_y)));
				float c_area = (((b_x - a_x) * (y - a_y)) - ((x - a_x) * (b_y - a_y)));
				float beta = b_area / area;
				float gamma = c_area / area;
				float alpha = 1 - beta - gamma;

            float depth = (alpha * a_z) + (beta * b_z) + (gamma * c_z);
            depth = (depth + 1) / 2; // map depth from [-1, 1] to [0, 1]

            if (!((alpha < 0) || (beta < 0) || (gamma < 0))) {
               if (color_mode == 0) {
                  if (depth_arr[y * g_width + x] <= depth) {
                     depth_arr[y * g_width + x] = depth;
                     image->setPixel(x, y, 0, 225 * depth, 0);
                  }
               }
               else if (color_mode == 1) {
                  bool on_edge = (alpha < 0.05 || beta < 0.05 || gamma < 0.05);
                  if (on_edge && (depth_arr[y * g_width + x] <= depth)) {
                     depth_arr[y * g_width + x] = depth;
                     image->setPixel(x, y, 0, 225 * depth, 0);
                  }
               }
				}
         }
      }

      //cout << i0 << ", " << i1 << ", " << i2 << endl;
   }

   //write out the image
   image->writeToFile(imgName);

	return 0;
}
