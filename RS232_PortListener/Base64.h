/*
@author  Ali Yavuz Kahveci aliyavuzkahveci@gmail.com
* @version 1.0
* @since   09-07-2018
* @Purpose: Provides functionality to encode or decode a data stream required for transmitting non-printable data
*/

#include <string>
#include <iostream>

namespace RS232
{
	class Base64
	{
	public:
		static std::string Encode(const unsigned char* plainData, unsigned int dataLength);
		static std::string Decode(const std::string& encodedStr);

		virtual ~Base64();

	private:
		/*to protect the class from being copied*/
		Base64() = delete;
		Base64(const Base64&) = delete;
		Base64& operator=(const Base64&) = delete;
		Base64(Base64&&) = delete;
		Base64& operator=(Base64&) = delete;
		/*to protect the class from being copied*/
	};
}

