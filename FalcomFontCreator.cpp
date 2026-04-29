/* example1.c                                                      */
/*                                                                 */
/* This small program shows how to print a rotated string with the */
/* FreeType 2 library.                                             */


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ft2build.h>
#include<stdint.h>
#include FT_FREETYPE_H
#include <vector>
#include <iostream>
#include <map>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <filesystem>
#include <istream>
#include <inicpp.h>
#include <unordered_map>


#include <unicode/unistr.h>
#include <unicode/ucnv_cb.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <unicode/chariter.h>

struct config {
	short resolution;
	int size;
	std::string font, itf_ref = "none";
	int base;
	int NbChar;
	short SpaceBetweenCharacters, SpaceWidth;
};

config conf;

std::vector<unsigned char> intToByteArray(int k){
	std::vector<unsigned char> res(4);
	res[0] =  k & 0x000000ff;
	res[1] = (k & 0x0000ff00) >> 8;
	res[2] = (k & 0x00ff0000) >> 16;
	res[3] = (k) >> 24;
	return res;
}
std::vector<unsigned char> shortToByteArray(short k){
	std::vector<unsigned char> res(2);
	res[0] =  k & 0x000000ff;
	res[1] = (k & 0x0000ff00) >> 8;

	return res;
}

int vectorToInt(std::vector<unsigned char> nb, int addr){
	int result = 0;
	int cnt = 0;
	for (std::vector<unsigned char>::const_iterator i = nb.begin()+addr; i != nb.begin()+addr+4; ++i){
		result = result + (((*i) & 0x000000ff) << (cnt*8));
		cnt++;
		}
	return result;
}
short vectorToShort(std::vector<unsigned char> nb, int addr){
	int result = 0;
	int cnt = 0;
	for (std::vector<unsigned char>::const_iterator i = nb.begin()+addr; i != nb.begin()+addr+2; ++i){
		result = result + (((*i) & 0x000000ff) << (cnt*8));
		cnt++;
		}
	return result;
}


void draw_bitmap(FT_GlyphSlot slot, FT_Face face, std::vector<uint8_t> &buffer_letter, const int baseline_offset) {
    FT_Bitmap* bitmap = &slot->bitmap;

    // 1. Force the width to be EVEN to prevent 45-degree shearing
    int fixed_width = (slot->metrics.horiAdvance >> 6);
    if (fixed_width <= 0) fixed_width = (bitmap->width > 0) ? bitmap->width : 8;
    if (fixed_width % 2 != 0) fixed_width++;

    uint8_t line_number = (uint8_t)bitmap->rows;

    // 2. Vertical Alignment logic
    int offset_y = (face->size->metrics.ascender >> 6) - slot->bitmap_top + baseline_offset;
    if (offset_y < 0) offset_y = baseline_offset;

    uint8_t byteoffy1 = (offset_y & 0xFF00) >> 8;
    uint8_t byteoffy2 = offset_y & 0xFF;

    // 3. Header Construction
    // 'b' is the total advance, 'fixed_width' is the pixel stride
    short b = (short)fixed_width;
    uint8_t byteb1 = (b & 0xFF00) >> 8;
    uint8_t byteb2 = b & 0xFF;

    // Note: The 7th byte is slot->bitmap_left to help the renderer if needed
    uint8_t byteArray[] = { (uint8_t)fixed_width, 0, line_number, 0, byteoffy2, byteoffy1, (uint8_t)slot->bitmap_left, 0, byteb2, byteb1, 0x01, 0x0 };
    for (int h = 0; h < 12; h++) buffer_letter.push_back(byteArray[h]);

    // 4. Pixel Processing Loop
    for (int q = 0; q < (int)line_number; q++) {
        uint8_t cur_pix_to_render = 0x0;

        for (int p = 0; p < fixed_width; p++) {
            uint8_t pixel = 0;

            // Map grid position to the 'ink' bitmap
            int bitmap_col = p - slot->bitmap_left;
            if (bitmap_col >= 0 && bitmap_col < (int)bitmap->width) {
                pixel = bitmap->buffer[q * bitmap->pitch + bitmap_col];
            }

            if (p % 2 == 0) {
                // First pixel (Even): Store in lower 4 bits
                cur_pix_to_render = (pixel & 0xF0) >> 4;
            } else {
                // Second pixel (Odd): Combine with first and PUSH byte
                cur_pix_to_render |= (pixel & 0xF0);
                buffer_letter.push_back(cur_pix_to_render);
                cur_pix_to_render = 0x0; // Reset for next pair
            }
        }

        // Safety: Because fixed_width is forced even, we should never
        // have a 'hanging' pixel here. Every row ends on a full byte.
    }
}



std::vector<uint8_t> draw_character(int character_code, FT_Face face, const int baseline_offset) {
    std::vector<uint8_t> result;

    FT_Load_Glyph( face, character_code, FT_LOAD_RENDER );
	FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );

    // Pass the entire slot and face to access metrics
    draw_bitmap(face->glyph, face, result, baseline_offset);

    return result;
}


struct character {
  int addr;
  int code;

  character(int a, int c){
	  addr = a;
	  code = c;

  }
  bool operator<(const character& rhs) const
    {
        if (addr < rhs.addr)
        {
           return true;
        }
        else return false;
    }
	bool operator==(const character& rhs) const
    {
        if (code == rhs.code)
        {
           return true;
        }
        else return false;
    }

};


void create_node(std::vector<character> &characters, std::vector<int> &pile_of_additional_characters, int code, int idx, int up, int low){ //each node, creates two nodes, left has lower code and right has higher,

	int low_idx = (low+code)/2;
	int high_idx = (up+code)/2;

	bool isAlreadyThere = std::find(characters.begin(), characters.end(), character(idx, code)) != characters.end();

	if (!isAlreadyThere) characters.push_back(character(idx, pile_of_additional_characters[code]));
	else characters.push_back(character(idx, -1));

	if (low_idx == high_idx) {

		if (pile_of_additional_characters[low_idx] > pile_of_additional_characters[code])
			characters.push_back(character(2*idx+1, pile_of_additional_characters[low_idx]));

		else
			characters.push_back(character(2*idx, pile_of_additional_characters[low_idx]));

		return;
	}

		if (pile_of_additional_characters[low_idx] < pile_of_additional_characters[code]){
			create_node(characters, pile_of_additional_characters, low_idx, 2*idx, code, low);
		}

		if (pile_of_additional_characters[high_idx] > pile_of_additional_characters[code]){

			create_node(characters, pile_of_additional_characters, high_idx, 2*idx+1, up, code);
		}

}
int main( int     argc,
      char**  argv )
{

	std::string str;
	std::size_t idx = -1;
	int code1 = 0;

	ini::IniFile myIni;
	myIni.load("config_font.ini");

	std::cout << "Reading config file..." << std::endl;
	if (myIni.count("general") > 0) {
		conf.resolution = myIni["general"]["Resolution"].as<int>();
		conf.size = myIni["general"]["FontSize"].as<int>();
		conf.base = myIni["general"]["Base"].as<int>();
		conf.NbChar = myIni["general"]["NbChar"].as<int>();
		conf.SpaceWidth = myIni["general"]["SpaceWidth"].as<int>();
		conf.SpaceBetweenCharacters = myIni["general"]["SpaceBetweenCharacters"].as<int>();
		conf.font = myIni["general"]["Font"].as<std::string>();
		if (myIni["general"].count("ITFReferenceFile") > 0) {
			conf.itf_ref = myIni["general"]["ITFReferenceFile"].as<std::string>();
		}
	}

	std::unordered_map<int, int> replacement_chars = {};
	if (myIni.count("Replacements") > 0) {
		for (auto it = myIni["Replacements"].begin(); it != myIni["Replacements"].end(); it++) {
			std::string to_replace = it->second.as<std::string>();
			std::string to_be_replaced = it->first;
			icu::UnicodeString src(to_replace.c_str(), "utf8"); //read as utf8
			icu::UnicodeString src2(to_be_replaced.c_str(), "utf8"); //read as utf8
			replacement_chars[src2.char32At(0)] = src.char32At(0);
		}
	}
	/*while (std::getline(file, str))
    {
			if (idx=str.find("FontSize")!=-1) {
			std::string size_str = str.substr(str.find("=")+1,str.length()-1);
			size = std::stoi(size_str);
			}
			else if (idx=str.find("Resolution")!=-1) {	 resolution = (uint8_t) std::stoi(str.substr(str.find("=")+1,str.length()));}
			else if (idx=str.find("Font")!=-1) font = str.substr(str.find("=")+1,str.length());
			else if (idx=str.find("Base")!=-1) base = std::stoi(str.substr(str.find("=")+1,str.length()));
			else if (idx=str.find("NbChar")!=-1) NbChar = std::stoi(str.substr(str.find("=")+1,str.length()));
			else if (idx=str.find("SpaceWidth")!=-1) SpaceWidth = std::stoi(str.substr(str.find("=")+1,str.length()));
			else if (idx=str.find("SpaceBetweenCharacters")!=-1) SpaceBetweenCharacters = std::stoi(str.substr(str.find("=")+1,str.length()));
			else if (idx=str.find("ITFReferenceFile")!=-1) itf_ref = str.substr(str.find("=")+1,str.length());

    }*/
	std::cout << "Resolution " << conf.resolution << std::endl;
	std::cout << "Font " << conf.font << std::endl;
	std::cout << "Base " << conf.base << std::endl;
	std::cout << "NbChar " << conf.NbChar << std::endl;
	std::cout << "FontSize " << conf.size << std::endl;
	std::cout << "ITFReferenceFile " << conf.itf_ref << std::endl;
	std::cout << std::endl;




	std::ifstream itf_file(conf.itf_ref, std::ios::binary);
	std::vector<unsigned char> buffer_font_reference_file;
	std::vector<character> reference_characters;
	bool itf_exists = std::filesystem::exists(conf.itf_ref);
	if (itf_exists){
		if ( itf_file ) {
			printf("Reference font file found, reading its content...\n");
			buffer_font_reference_file = std::vector<unsigned char>((std::istreambuf_iterator<char>(itf_file)),
			std::istreambuf_iterator<char>());


			unsigned int nb_int = vectorToInt(buffer_font_reference_file, 12);
			unsigned int nb_char = vectorToInt(buffer_font_reference_file, 8);
			int addr_start = 0x10 + (nb_int-1) * 4;
			int addr_end = addr_start + (nb_char) * 4;
			printf("Found %d characters. It might take a while, please be patient.\n", nb_char);
			for (int i = 0; i < nb_char; i++){

				unsigned int addr = vectorToInt(buffer_font_reference_file, addr_start + i*8 + 4);
				unsigned int code = vectorToInt(buffer_font_reference_file, addr_start + i*8 + 0);
				//printf("Character %x found at address %x.\n", code, addr);
				character c = character(addr, code);
				reference_characters.push_back(c);
			}
			std::sort(reference_characters.begin(), reference_characters.end());
			printf("Done reading the content.\n");

		}


	}
	itf_file.close();
	std::vector<int> pile_of_additional_characters;
	for (int i = 0; i< conf.NbChar; i++){
		pile_of_additional_characters.push_back(i);
	}

	std::vector<unsigned char> buffer_font_file;
	std::sort(pile_of_additional_characters.begin(), pile_of_additional_characters.end());



	std::vector<unsigned char> resolution_bytes = shortToByteArray(conf.resolution);

	int idk_what_that_is = 0x0D;
	std::vector<unsigned char> nb_char_bytes = intToByteArray(pile_of_additional_characters.size());
	std::vector<unsigned char> bytes_idk = intToByteArray(idk_what_that_is);
	buffer_font_file.push_back(0x01);
	buffer_font_file.push_back(0x01);
	buffer_font_file.insert(buffer_font_file.end(), resolution_bytes.begin(),resolution_bytes.end());

	buffer_font_file.insert(buffer_font_file.end(),nb_char_bytes.begin(), nb_char_bytes.end());
	buffer_font_file.insert(buffer_font_file.end(),nb_char_bytes.begin(), nb_char_bytes.end());
	buffer_font_file.insert(buffer_font_file.end(),bytes_idk.begin(), bytes_idk.end());

	for (int i = 0; i<idk_what_that_is-1; i++){
		std::vector<unsigned char> zero_bytes = intToByteArray(0);
		buffer_font_file.insert(buffer_font_file.end(),zero_bytes.begin(),zero_bytes.end());
	}


	std::vector<character> characters;

	int last_addr = 0;
	int pos_of_declaration_in_file;


	int first_code = pile_of_additional_characters.size()/2;
	idx = 0;
	pos_of_declaration_in_file = 0xC + idk_what_that_is*4 + 0x8*idx;
	auto it_max = max_element(std::begin(pile_of_additional_characters), std::end(pile_of_additional_characters));
	auto it_min = min_element(std::begin(pile_of_additional_characters), std::end(pile_of_additional_characters));
	create_node(characters, pile_of_additional_characters, first_code, 1, it_max - pile_of_additional_characters.begin(), it_min - pile_of_additional_characters.begin());

	std::sort(characters.begin(), characters.end());

	FT_Library    library;
	FT_Face       face;
	FT_GlyphSlot  slot;
	FT_Vector     pen;
	FT_Error      error;

	int           n, num_chars;
	std::string filename = conf.font;


	int glyph_code;
	int length = 0;

	error = FT_Init_FreeType( &library );
	error = FT_New_Face( library, filename.c_str(), 0, &face );
	const int baseline = conf.resolution * conf.base/0x20;
	int actual_resolution = conf.resolution * 530/0x40;
	error = FT_Set_Char_Size(face, conf.size, 0, actual_resolution, 0);
	slot = face->glyph;
	int off_y;
	std::vector<character>::iterator it = characters.begin();
	int globalOffset = 0;
	int end_drawing;
	int end_code;
	const FT_Byte*  file_base;


	std::vector<unsigned char> addr_section, drawing_section;
	int current_position = buffer_font_file.size() + characters.size() * 8;

	for (it = characters.begin(); it != characters.end(); it++){
		bool replaced_char = false;
		character current_char = *it;

		slot = face->glyph;
		uint32_t old_code = 0;

		if (replacement_chars.count(current_char.code) > 0) {
			old_code = current_char.code;
			current_char.code = replacement_chars[current_char.code];
			replaced_char = true;
		}


		int char_code = current_char.code;
		glyph_code = FT_Get_Char_Index(face, char_code );

		if (glyph_code == 0){

			//printf("The required character %x doesn't exist in the ttf font.\n", char_code);
			if (reference_characters.size() != 0){
				//printf("Trying to find it in the ITF reference font file...\n");
				bool character_found = false;
				for (int i = 0; i < reference_characters.size(); i++){
					if (reference_characters[i].code ==  char_code){
						int addr_end;
						if (i == reference_characters.size() - 1) addr_end = buffer_font_reference_file.size();
						else addr_end = reference_characters[i+1].addr;
						std::vector<unsigned char>::const_iterator first = buffer_font_reference_file.begin() + reference_characters[i].addr;
						std::vector<unsigned char>::const_iterator last = buffer_font_reference_file.begin() + addr_end;
						std::vector<unsigned char> letter_bytes(first, last);
						if (letter_bytes.size() > 8){
							unsigned short y = vectorToShort(letter_bytes, 4);
							unsigned short x = vectorToShort(letter_bytes, 0);
							unsigned short rows = vectorToShort(letter_bytes, 2);
							unsigned short offset_y = 1 + baseline - rows;
							if (offset_y < 0) offset_y = 0;
							unsigned char byteoffy1 = (offset_y & 0xFF00)>>8;
							unsigned char byteoffy2 = offset_y & 0xFF;


							letter_bytes[4] = byteoffy2;
							letter_bytes[5] = byteoffy1;
						}


						std::vector<unsigned char> code_bytes = intToByteArray(reference_characters[i].code);
						std::vector<unsigned char> addr_bytes = intToByteArray(current_position);
						addr_section.insert(addr_section.end(),code_bytes.begin(), code_bytes.end());
						addr_section.insert(addr_section.end(),addr_bytes.begin(),addr_bytes.end());

						current_position += letter_bytes.size();
						drawing_section.insert(drawing_section.end(),letter_bytes.begin(), letter_bytes.end());
						character_found = true;
						//printf("Found it!\n");
						break;
					}
				}
				if (!character_found){
					//printf("No luck!\n");

					std::vector<unsigned char> code_bytes = intToByteArray(current_char.code);
					std::vector<unsigned char> addr_bytes = intToByteArray(buffer_font_file.size() + characters.size() * 8);
					addr_section.insert(addr_section.end(),code_bytes.begin(), code_bytes.end());
					addr_section.insert(addr_section.end(),addr_bytes.begin(),addr_bytes.end());
				}


			}
			else{
				std::vector<unsigned char> code_bytes = {};
				if (replaced_char)
					code_bytes = intToByteArray(old_code);
				else
					code_bytes = intToByteArray(current_char.code);
				std::vector<unsigned char> addr_bytes = intToByteArray(buffer_font_file.size() + characters.size() * 8);
				addr_section.insert(addr_section.end(),code_bytes.begin(), code_bytes.end());
				addr_section.insert(addr_section.end(),addr_bytes.begin(),addr_bytes.end());

			}
		}
		else{

		    std::vector<unsigned char> letter = draw_character(glyph_code, face, baseline);
			std::vector<unsigned char> code_bytes = {};
			if (replaced_char)
				code_bytes = intToByteArray(old_code);
			else
				code_bytes = intToByteArray(current_char.code);
			std::vector<unsigned char> addr_bytes = intToByteArray(current_position);
			addr_section.insert(addr_section.end(),code_bytes.begin(), code_bytes.end());
			addr_section.insert(addr_section.end(),addr_bytes.begin(),addr_bytes.end());


			current_position += letter.size();
			drawing_section.insert(drawing_section.end(),letter.begin(), letter.end());

		}

	}
	buffer_font_file.insert(buffer_font_file.end(),addr_section.begin(), addr_section.end());
	buffer_font_file.insert(buffer_font_file.end(),drawing_section.begin(), drawing_section.end());
	error = FT_Done_FreeType( library );

	std::ofstream writeFontFile;
	writeFontFile.open("font.itf", std::ios::out | std::ios::binary);
	if ( writeFontFile ) {
	writeFontFile.write((const char*)&buffer_font_file[0], buffer_font_file.size());
	writeFontFile.close();
	std::cout << "SUCCESS!";
	}
	else {
	   std::cout << "FAIL!";

	}
	scanf("%d");

	  return 0;
}

/* EOF */
