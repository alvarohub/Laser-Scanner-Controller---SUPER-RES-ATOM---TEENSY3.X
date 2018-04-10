#include "renderer2D.h"

namespace Renderer2D {

    // ======================= DEFAULT STATE VARIABLE DEFINITIONS  =======================
    // NOTE: default parameters should be in a "defaultParameters" file (Json or XML?). For the
    // time being, they are in Utils.h
    P2 center(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
    float angle = 0;
    float scaleFactor = 1.0;

    uint16_t sizeBlueprint = 0; // this would not be necessary if using an STL container. It is
    // just the size of the current bluepring array, modified and set when drawing a figure (see
    // graphic primitives)

    PointBuffer bluePrintArray;

    void clearBlueprint() {
        sizeBlueprint = 0;
        DisplayScan::stopSwapping();
        DisplayScan::resizeBuffer(sizeBlueprint);
        DisplayScan::startSwapping();
    }
    uint16_t getSizeBlueprint() {
        return(sizeBlueprint);
    } // mainly for check

    const P2 getLastPoint() {
        return(bluePrintArray[sizeBlueprint-1]);
    }

    void addToBlueprint(const P2 _newPoint) { // add point and increment index:
        if (sizeBlueprint<MAX_NUM_POINTS) bluePrintArray[sizeBlueprint++] = _newPoint;
        // otherwise do nothing
    }

    void writeInBluePrintArray(uint16_t _index, const P2 &_newPoint) {
        // Could be used to overwrite a figure, but normally we would use addToBluePrint(...)
        if (_index<MAX_NUM_POINTS) bluePrintArray[_index] = _newPoint;
    }

    // ======= RENDERING with CURRENT POSE TRANSFORMATION =====================================
    void renderFigure() {
        // * NOTE: this needs to be called when changing the figure or number of points,
        // but also after modifying pose to avoid approximation errors.

        DisplayScan::stopSwapping(); // necessary during the re-writting to the hidden buffer
        // Draw the figure with proper translation, rotation and scale on the "hidden" buffer:

        for (uint16_t i = 0; i < sizeBlueprint; i++) {
            P2 point(bluePrintArray[i]);
            // In order: resize, rotate and then translate (resize and rotate are commutative)
            point.scale(scaleFactor);
            point.rotate(angle);
            point.translate(center);
            //point.constrainPos(); won't do that - prefer to compute with floats outside range, but the Scanner setMirrorsTo method
            // will take care of the contrain.
            DisplayScan::writeOnHiddenBuffer(i, point); // the "bridge" method between the renderer and the displaying engine!
        }

        DisplayScan::resizeBuffer(sizeBlueprint); // could be merged with resumeSwapping?
        //PRINTLN(sizeBlueprint);

        DisplayScan::startSwapping();// render is over... the ISR will swap buffers if needed.
    }

}
