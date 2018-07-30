#include "Base64.h"
#include "RS232_Util.h"

#include <cctype>

namespace RS232
{
	static const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	static inline bool is_base64(unsigned char c)
	{
		return (std::isalnum(c) || (c == ASCII_PLS) || (c == ASCII_SLH));
	}

	std::string Base64::Encode(const unsigned char* plainData, unsigned int dataLength)
	{
		std::string encodedStr;
		int i = 0;
		int j = 0;
		unsigned char char_array_3[3];
		unsigned char char_array_4[4];

		while (dataLength--)
		{
			char_array_3[i++] = *(plainData++);
			if (i == 3)
			{
				char_array_4[0] = (char_array_3[0] & ASCII_LSUD) >> 2;
				char_array_4[1] = ((char_array_3[0] & ASCII_ETX) << 4) + ((char_array_3[1] & ASCII_LSET) >> 4);
				char_array_4[2] = ((char_array_3[1] & ASCII_SI) << 2) + ((char_array_3[2] & ASCII_LCAG) >> 6);
				char_array_4[3] = char_array_3[2] & ASCII_QSTN;

				for (i = 0; (i <4); i++)
					encodedStr += base64_chars[char_array_4[i]];

				i = 0;
			}
		}

		if (i)
		{
			for (j = i; j < 3; j++)
				char_array_3[j] = ASCII_NULL;

			char_array_4[0] = (char_array_3[0] & ASCII_LSUD) >> 2;
			char_array_4[1] = ((char_array_3[0] & ASCII_ETX) << 4) + ((char_array_3[1] & ASCII_LSET) >> 4);
			char_array_4[2] = ((char_array_3[1] & ASCII_SI) << 2) + ((char_array_3[2] & ASCII_LCAG) >> 6);
			char_array_4[3] = char_array_3[2] & ASCII_QSTN;

			for (j = 0; (j < i + 1); j++)
				encodedStr += base64_chars[char_array_4[j]];

			while ((i++ < 3))
				encodedStr += ASCII_EQ;
		}

		return encodedStr;
	}

	std::string Base64::Decode(const std::string& encodedStr)
	{
		std::string plainStr;
		int in_len = encodedStr.size();
		int i = 0;
		int j = 0;
		int in_ = 0;
		unsigned char char_array_4[4], char_array_3[3];

		while (in_len-- && (encodedStr[in_] != ASCII_EQ) && is_base64(encodedStr[in_]))
		{
			char_array_4[i++] = encodedStr[in_]; in_++;
			if (i == 4)
			{
				for (i = 0; i <4; i++)
					char_array_4[i] = base64_chars.find(char_array_4[i]);

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & ASCII_0) >> 4);
				char_array_3[1] = ((char_array_4[1] & ASCII_SI) << 4) + ((char_array_4[2] & ASCII_LESS) >> 2);
				char_array_3[2] = ((char_array_4[2] & ASCII_ETX) << 6) + char_array_4[3];

				for (i = 0; (i < 3); i++)
					plainStr += char_array_3[i];

				i = 0;
			}
		}

		if (i) {
			for (j = i; j < 4; j++)
				char_array_4[j] = 0;

			for (j = 0; j < 4; j++)
				char_array_4[j] = base64_chars.find(char_array_4[j]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & ASCII_0) >> 4);
			char_array_3[1] = ((char_array_4[1] & ASCII_SI) << 4) + ((char_array_4[2] & ASCII_LESS) >> 2);
			char_array_3[2] = ((char_array_4[2] & ASCII_ETX) << 6) + char_array_4[3];

			for (j = 0; (j < i - 1); j++)
				plainStr += char_array_3[j];
		}

		return plainStr;
	}
}
