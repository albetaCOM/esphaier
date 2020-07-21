/**
* Create by Miguel Ángel López on 20/07/19
* Modified by Alba Prades on 21/07/20
**/

#ifndef HAIER_ESP_HAIER_H
#define HAIER_ESP_HAIER_H

#include "esphome.h"

using namespace esphome;
using namespace esphome::climate;

// Updated read offset
#define TEMPERATURE_OFFSET   	21

#define COMMAND_OFFSET       	17

#define MODE_OFFSET 			23
	#define MODE_SMART 			0
	#define MODE_COOL 			1
	#define MODE_HEAT 			2
	#define MODE_ONLY_FAN 		3
	#define MODE_DRY 			4

	#define FAN_SPEED   		25
	#define FAN_MIN     		2
	#define FAN_MIDDLE  		1
	#define FAN_MAX     		2
	#define FAN_AUTO    		3

	#define SWING        		27
	#define SWING_OFF          	0
	#define SWING_VERTICAL     	1
	#define SWING_HORIZONTAL   	2
	#define SWING_BOTH

	#define LOCK        		28
	#define LOCK_ON     		80
	#define LOCK_OFF    		00

// Updated read offset
#define POWER_OFFSET       		17
	#define POWER_ON    		3
	#define POWER_OFF   		2
	#define POWER_ON_2    		3//25
	#define POWER_OFF_2   		2//24


#define FRESH       			31
	#define FRESH_ON    		1
	#define FRESH_OFF   		0

// Updated read offset
#define SET_TEMPERATURE_OFFSET 12

#define CRC_OFFSET 				45

// Control commands
#define CTR_POWER_OFFSET		13
	#define CTR_POWER_ON		0x01
	#define CTR_POWER_OFF		0x00
	
#define CTR_SET_POINT			12




// temperatures supported by AC system
#define MIN_SET_TEMPERATURE 16
#define MAX_SET_TEMPERATURE 30

//if internal temperature is outside of those boundaries, message will be discarded
#define MIN_VALID_INTERNAL_TEMP 10
#define MAX_VALID_INTERNAL_TEMP 50

class Haier : public Climate, public PollingComponent {

private:

    byte lastCRC;
    byte data[47];
 	byte poll[15] = {0xFF,0xFF,0x0A,0x40,0x00,0x00,0x00,0x00,0x00,0x01,0x4D,0x01,0x99,0xB3,0xB4};
    byte power_command[17]     = {0xFF,0xFF,0x0C,0x40,0x00,0x00,0x00,0x00,0x00,0x01,0x5D,0x01,0x00,0x01,0xAC,0xBD,0xFB};
	byte set_point_command[25]   = {0xFF,0xFF,0x14,0x40,0x00,0x00,0x00,0x00,0x00,0x01,0x60,0x01,0x09,0x08,0x25,0x00,0x02,0x03,0x00,0x06,0x00,0x0C,0x03,0x0B,0x70};


public:

    Haier() : PollingComponent(5 * 1000) {
        lastCRC = 0;
    }


    
    void setup() override {
        
        Serial.begin(9600);
    }

    void loop() override  {
        if (Serial.available() > 0) {
			ESP_LOGD("loop", "Serial data available");
       
			if (Serial.read() != 255) return;
			if (Serial.read() != 255) return;
			
			data[0] = 255;
			data[1] = 255;

            Serial.readBytes(data+2, sizeof(data)-2);

            readData();

		}
    }

    void update() override {
            
        Serial.write(poll, sizeof(poll));
        auto raw = getHex(poll, sizeof(poll));
        ESP_LOGD("Haier", "POLL: %s ", raw.c_str());
    }

protected:
    ClimateTraits traits() override {
        auto traits = climate::ClimateTraits();
        traits.set_supports_away(false);
        traits.set_supports_auto_mode(true);
        traits.set_supports_heat_mode(true);
        traits.set_supports_cool_mode(true);
        traits.set_visual_min_temperature(MIN_SET_TEMPERATURE);
        traits.set_visual_max_temperature(MAX_SET_TEMPERATURE);
        traits.set_visual_temperature_step(1.0f);
        traits.set_supports_current_temperature(true);

        return traits;
    }

public:

    void readData() {


        auto raw = getHex(data, sizeof(data));
        ESP_LOGD("Haier", "Readed message: %s ", raw.c_str());


        byte check = data[CRC_OFFSET];

        getChecksum(data, sizeof(data));

        if (check != data[CRC_OFFSET]) {
            ESP_LOGW("Haier", "Invalid checksum");
            return;
        }


        lastCRC = check;

        current_temperature = data[TEMPERATURE_OFFSET] + 16;
        target_temperature = data[SET_TEMPERATURE_OFFSET] + 16;

        if(current_temperature < MIN_VALID_INTERNAL_TEMP || current_temperature > MAX_VALID_INTERNAL_TEMP 
            || target_temperature < MIN_SET_TEMPERATURE || target_temperature > MAX_SET_TEMPERATURE){
            ESP_LOGW("Haier", "Invalid temperatures");
            return;
        }


        if (data[POWER_OFFSET] == POWER_OFF || data[POWER_OFFSET] == POWER_OFF_2) {
            mode = CLIMATE_MODE_OFF;

        } else {

            switch (data[MODE_OFFSET]) {
                case MODE_COOL:
                    mode = CLIMATE_MODE_COOL;
                    break;
                case MODE_HEAT:
                    mode = CLIMATE_MODE_HEAT;
                    break;
                default:
                    mode = CLIMATE_MODE_AUTO;
            }


        }

        this->publish_state();

    }


    void control(const ClimateCall &call) override {

        if (call.get_mode().has_value()) {
            switch (call.get_mode().value()) {
                case CLIMATE_MODE_OFF:
                    power_command[CTR_POWER_OFFSET] = CTR_POWER_OFF;
                    break;
                case CLIMATE_MODE_AUTO:
					power_command[CTR_POWER_OFFSET] = CTR_POWER_ON;

                    //data[MODE_OFFSET] = MODE_SMART;
                    break;
                case CLIMATE_MODE_HEAT:
                    power_command[CTR_POWER_OFFSET] = CTR_POWER_ON;
                    
					//data[MODE_OFFSET] = MODE_HEAT;
                    break;
                case CLIMATE_MODE_COOL:
                    power_command[CTR_POWER_OFFSET] = CTR_POWER_ON;
                    
					//data[MODE_OFFSET] = MODE_COOL;
                    break;
            }
			sendData(power_command, sizeof(power_command));
		}
			
		if (call.get_target_temperature().has_value()) {
			set_point_command[CTR_SET_POINT] = (uint16) call.get_target_temperature().value() - 16;
			sendData(set_point_command, sizeof(set_point_command));
 		}
   }


    void sendData(byte * message, byte size) {

        byte crc = getChecksum(message, size);
        Serial.write(message, size-1);
        Serial.write(crc);

        auto raw = getHex(message, size);
        ESP_LOGD("Haier", "Sended message: %s ", raw.c_str());

    }

    String getHex(byte * message, byte size) {

        String raw;

        for (int i=0; i < size; i++){
            raw += "\n" + String(i) + "-" + String(message[i]);

        }
        raw.toUpperCase();

        return raw;


    }

    byte getChecksum(const byte * message, size_t size) {
        byte position = size - 1;
        byte crc = 0;

        for (int i = 2; i < position; i++)
            crc += message[i];

        return crc;

    }




};


#endif //HAIER_ESP_HAIER_H