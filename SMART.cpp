#include <vector>
#include <cstdint>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/hdreg.h>
#include <iostream>
#include <cstring>
#include <iomanip>

#define ATA_SMART_CMD        0xB0
#define ATA_SMART_READ_DATA  0xD0
#define ATA_SMART_CYL_LOW    0x4F
#define ATA_SMART_CYL_HI     0xC2





// Represents a single SMART attribute from the device
struct SmartAttributes {
	uint8_t id;
	uint16_t flag;
	uint8_t currentValue;
	uint8_t worst;
	uint8_t threshhold;
	uint64_t rawValue;
	std::string type;
	std::string update;

	std::string when_failed; 
	/* when_failed = Attribute passed or failed returns std::string
	  "-"				= no meaning threshold
	  "FAILING NOW"		= current value below threshold
	  "In The Past"		= worst value below threshold (failed in the past)
	*/
};


// Represents a SMART-capable storage device.
class SMARTDevice {
	private:
		std::string device_path;
		std::vector<SmartAttributes> attributes;


		// Push Vector of attributes to class member
		// Used internally only so private
		void addAttribute(const SmartAttributes& attr) {
			attributes.push_back(attr);
		};


		// Get Raw_value
		uint64_t get_raw_value_from_buffer(const uint8_t* raw) const {
			uint64_t raw_value = 0;
    
			for (int i = 0; i < 6; i++) {
				raw_value |= ((uint64_t)raw[i]) << (i * 8);
			}
	 
			return raw_value;
		};	


		// Get Type
		std::string get_type(const uint16_t flag) const {
			if(flag & 0x00001) {
				return "pre-fail";
			}

			else {
				return "old_age";
			}
		};


		// Get Updated
		std::string get_updated(const uint16_t flag) const {
			if((flag & 0x0002) == 0x0002) {
				return "Always";
			
			}
			else {
				return "Offline";
			};
		}


		// Get when_Failed
		std::string get_when_failed(uint8_t current, uint8_t worst, uint8_t threshold, uint16_t flag) {
			
			// no meaningfull threshold basically
			if(threshold == 0) return "-";

			bool is_prefail = (flag & 0x0001);

			// Current value is below threshold
			if(is_prefail && current <= threshold) {
				return "Failing Now";
			};

			// Worst value is below threshold so was below in the past but not now
			if(worst <= threshold) {
				return "In The Past";
			}


			 return "pass";

		};

	public:

		// Get Vector of Smart Attributes
		// returned as refrence so it wont copy
		// also const so returned value cant be modified
		 const std::vector<SmartAttributes>& get_attributes() const{
			return this->attributes;
		}
		 



		 // Used member initializer
		SMARTDevice(const std::string& devicePath) : device_path(devicePath) {} 

		bool readSmartAttributes() {
			int file_disc = open(this->device_path.c_str(), O_RDONLY | O_NONBLOCK);
			if(file_disc < 0) {
				perror("Failed to open device");
				return false;
			}


			unsigned char args[4 + 512] = {0};
			args[0] = 0xB0;		// Command byte
			args[1] = 0x01;		// NSECTOR byte
			args[2] = 0xD0;		// Feature byte
			args[3] = 0x01;		// SECTOR byte

			//HDIO_DRIVE_CMD is to execute a special drive command as hdreg.h explains it
			if(ioctl(file_disc, HDIO_DRIVE_CMD, args) < 0) {
				perror("HDIO_DRIVE_CMD failed");
				close(file_disc);
				return false;
			};


			
			unsigned char thresh_args[4 + 512] = {0};
			thresh_args[0] = 0xB0;  
			thresh_args[1] = 0x01;  
			thresh_args[2] = 0xD1;	// Different command for threshold data
			thresh_args[3] = 0x01;	

			if(ioctl(file_disc, HDIO_DRIVE_CMD, thresh_args) < 0) {
				perror("Failed to read SMART threshholds");
				close(file_disc);
				return false;
			}

			unsigned char *data = args + 4;
			//30 total attributes

			unsigned char *thresh_data = thresh_args + 4;
			for(int i = 0; i < 30; ++i) {

				//each Attribute is 12 bytes
				//Apparently attribute start at 2 bytes?
				int offset = 2 + i * 12;
		

				uint8_t attr_id		= data[offset];
				uint8_t flag_lo		= data[offset + 1];
				uint8_t flag_hi		= data[offset + 2];
				uint16_t flag		= (flag_lo | flag_hi << 8);
				uint8_t current		= data[offset + 3];
				uint8_t worst		= data[offset + 4];
				uint8_t raw[6];

				std::memcpy(raw, &data[offset + 5], 6);  

				if(attr_id == 0) continue;

				uint8_t threshold = 0;
				for (int j = 0; j < 30; ++j) {
					int toffset = 2 + j * 12;
					uint8_t tid = thresh_data[toffset]; // FIXED HERE
					if (tid == attr_id) {
						threshold = thresh_data[toffset + 1];
						break;
					}
				}

				// Uncomment bellow to print Attributes
				
				//std::cout 
				//<< "ID: "<< (int)attr_id
				//<< "  Attribute Name:" << get_smart_attr_name(attr_id) 
				//<< "  Flags: "		<<(int)flag
				//<< "  VALUE: "		<< (int)current
				//<< "  WORST: "		<< (int)worst
				//<< "  THRESH: "		<< (int)threshold
				//<< "  Raw_Value: "	<<get_raw_value_from_buffer(raw)
				//<< "  Type: "		<<get_type(flag)
				//<< "  Update: "		<<get_updated(flag)
				//<< "  When Failed: "<<get_when_failed(current, worst, threshold, flag)
				//<<std::endl;
				
				
				SmartAttributes attr = {
					attr_id,
					flag,
					current,
					worst,
					threshold,
					get_raw_value_from_buffer(raw),
					get_type(flag),
					get_updated(flag),
					get_when_failed(current, worst, threshold, flag)
				};
				addAttribute(attr);

			};
			return true;
			close(file_disc);
		};

		


};

