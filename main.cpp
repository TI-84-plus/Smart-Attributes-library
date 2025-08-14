
#include <iostream>
#include <string>
#include "SMART.cpp"




int main() {
		
	//SMARTDevice per device, Constructor takes block device path
	SMARTDevice SmartManager("/dev/sda");

	//After initializing readSmartAttributes()
	SmartManager.readSmartAttributes();
	
	//returns a std::vector as ref which contains SmartAttributes
	//std::vector<SmartAttributes>
	//
	for(int i =0; i < SmartManager.get_attributes().size(); i++) {
		//id for example
		//printed as int because .id = uint8_t
		std::cout<< (int)SmartManager.get_attributes()[i].id<<std::endl;
	}
	
    return 0;
}
