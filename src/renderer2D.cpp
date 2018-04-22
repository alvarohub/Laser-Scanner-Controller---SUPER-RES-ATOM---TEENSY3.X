#include "renderer2D.h"

namespace Renderer2D {

  // ======================= DEFAULT STATE VARIABLE DEFINITIONS  =======================
  // NOTE: default parameters should be in a "defaultParameters" file (Json or XML?). For the
  // time being, they are in Utils.h
  P2 center(0,0); // note (0,0) in "renderer" coordinates is the center of mirrors.
  float angle = 0;
  float scaleFactor = 1.0;

  uint16_t sizeBlueprint = 0; // this would not be necessary if using an STL container. It is
  // just the size of the current bluepring array, modified and set when drawing a figure (see
  // graphic primitives)

  P2 bluePrintArray[MAX_NUM_POINTS];// PointBuffer bluePrintArray
  P2 frameBuffer[MAX_NUM_POINTS];   // the rendered, clipped points (do a P2i).
  // NOTE: frameBuffer is an auxiliary LOCAL variable, we could write directly to the hidden display buffer,
  // but this way we may have further processing done

  uint16_t getSizeBlueprint() {
    return(sizeBlueprint);
  } // mainly for check

  const P2 getLastPoint() {
    return(bluePrintArray[sizeBlueprint-1]);
  }

  void addToBlueprint(const P2 &_newPoint) {
    // add point and increment index:
    if (sizeBlueprint<MAX_NUM_POINTS) bluePrintArray[sizeBlueprint++] = _newPoint;
    // otherwise do nothing
  }

  // ======= RENDERING with CURRENT POSE TRANSFORMATION =====================================
  void renderFigure() {
    // * NOTE: this needs to be called when changing the figure or number of points,
    // but also after modifying pose to avoid approximation errors.

    // Draw the figure with proper translation, rotation and scale on the "hidden" buffer:
    uint16_t numframeBufferPoints = 0;
    for (uint16_t i = 0; i < sizeBlueprint; i++) {
      P2 point(bluePrintArray[i]);

      // 1) The true render: in order: resize, rotate and then translate (resize and rotate are commutative)
      point.scale(scaleFactor); // equal to point = point*scaleFactor.
      point.rotate(angle);
      point.translate(center);

      // 2) The viewport transform [could be in another namespace/method]);
      // Map to the galvo limits [ROI parameters are passed here
      // because these do not belong to the Hardware namespace]:
      Hardware::Scanner::mapViewport(point, minX, maxX, minY, maxY);

      // 3) Before saving the "framebuffer", do the "vieport/clip transform:
      // * NOTE : we can choose here to either clip the points AND show them clipped, or
      // just NOT put them in the display buffer. I will use the second option here:
      if ( !Hardware::Scanner::clipLimits(point) ) {  // constrain to the galvo limits
        frameBuffer[i] = point;
        numframeBufferPoints++;
      }

    }

    //4) Finally, the "bridge" method between the renderer and the displaying engine:
    // * NOTE: the "frameBuffer" is the buffer of rendered, projected, viewported and clipped points,
    // and it is made of uint16_t points! (for the itme being, still floats, but use a TEMPLATE and a typedef...)
    // * NOTE 2 : this method will fill the current "hidden" buffer, and indicate the need to swap buffers:
    DisplayScan::setDisplayBuffer(frameBuffer, numframeBufferPoints);
  }

  void clearBlueprint() {
    sizeBlueprint = 0;
    renderFigure();
  }

}
