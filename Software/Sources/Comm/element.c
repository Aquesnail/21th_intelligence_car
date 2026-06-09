#include "element.h"
#include "debug_arch.h"

static float Ring[4] = {0.0f},angle_pitch=5.0f;

void Element_Init(void){
    Debug_Register("Ring1", DBG_FLOAT | STATIC_TYPE, &Ring[0]);
    Debug_Register("Ring2", DBG_FLOAT | STATIC_TYPE, &Ring[1]);
    Debug_Register("Ring3", DBG_FLOAT | STATIC_TYPE, &Ring[2]);
    Debug_Register("Ring4", DBG_FLOAT | STATIC_TYPE, &Ring[3]);
    /* 元素系统初始化 (如有需要) */
}
//adc0是左
 void Element_Handle(element_t *element,float adc_values[],float pitch,float roll,float yaw){
    float abs_pitch = (pitch > 0) ? pitch : -pitch;    
    switch(*element){
            case ELEMENT_STRAIGHT:
                if(adc_values[0]>Ring[0] && adc_values[2]>Ring[2] && adc_values[0]>Ring[2]&&(abs_pitch<angle_pitch)){
                    *element = ELEMENT_RING_LEFT_ENTRANCE;
                }
                break;
            case ELEMENT_RING_LEFT_ENTRANCE:
                break;
            case ELEMENT_RING_RIGHT_ENTRANCE:
                break;
            case ELEMENT_RING_LEFT_EXIT:
                break;
            case ELEMENT_RING_RIGHT_EXIT:
                break;
            case ELEMENT_SKIP:
                break;
            case ELEMENT_BARREL:
                break;
            case ELEMENT_WALL:
                break;
            case ELEMENT_CROSS:
                break;
             default:
                break;
        }
 }