#include "config.h"
#include "spi1_user.h"

void SPI1_Callback(){


}

void Main_Control_Task(){

    SPI1_Init(SPI1_Callback);
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Main_Control_Task_Init(){
     xTaskCreate(Main_Control_Task, "main_control", 1024, NULL, tskIDLE_PRIORITY + 4, NULL);
}