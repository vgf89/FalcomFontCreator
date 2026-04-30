# Changed in this fork:
- Use built-in font metrics to render glyphs (fixes spacing when processing JP fonts).
- Change "Base" setting to instead be an offset from the font's built-in baseline.
- Includes a config_font.ini for recreating the original Trails of Cold Steel JP font in HD (you will need to source the font yourself).
- Saves the resulting file to font.itf instead of font_us.itf.
- Add a Makefile to trivialize the build process
- Removed the static headers for FreeType2 etc, as that should be provided by your system
    - On Fedora, dependincies include `freetype-devel libicu-devel`

# このフォークでの変更点:
- グリフのレンダリングに内蔵のフォントメトリクスを使用（日本語フォント処理時の間隔の問題を修正）。
- 「Base」の設定を、フォント内蔵のベースラインからのオフセットに変更。
- 『英雄伝説 閃の軌跡』オリジナルの日本語フォントをHDで再現するための config_font.ini を同梱（フォント自体は各自で用意する必要があります）。
- 出力ファイルの保存先を font_us.itf ではなく font.itf に変更。
- ビルド工程を簡略化するため Makefile を追加。
- FreeType2 などの静的ヘッダーを削除（システム側のライブラリを使用するように変更）。
    - Fedora の場合、依存関係には `freetype-devel libicu-devel` が含まれます。


# FalcomFontCreator
A simple tool to generate a .itf font file from a .ttf/.otf file. Those fonts are used in recent Falcom games such as Trails of Cold Steel or Tokyo Xanadu.
Please don't take this tool "too" seriously, I didn't spend that much time studying it, I can't explain everything in the font file, but I have a general understanding of the file structure and thus the output file should work.

# How to use
Just fill the "config_font.ini" file then run the Font Creator.

Resolution=64 <= this is the resolution of the font, 64 should be "HD" enough

FontSize=512 <= the font size

Font=cuprum.ttf <= the path to the input TTF file

Base=26 <= something to play with to realign all the characters if a big one is misaligned (putting a greater value should help fixing the font)

NbChar=4095 <= 4095 means we will try to render the first 4095 characters in unicode using the TTF file (the higher the number, the heavier the itf file gets)

# Additional characters from existing ITF file

If for some reason there is a need for very special characters that are not in a ttf file but inside a itf instead, you can add the following line to the config_font.ini:

ITFReferenceFile= "itf font file path"

This will tell the tool to pick the characters inside this font when they are not present in the provided TTF file. However, there is a problem of alignment. To my understanding, there is no way to know the base of the character from the itf font data. Therefore you will need to align those special characters manually inside the resulting itf file.

To do that, first look for the character inside the itf file by looking for its unicode in the first section. Next to its code is the address inside the file. When going to this address (addr), you can adjust the position of the character by editing the value at addr + 4 (for y axis) and addr + 6 (for x axis).

From 1.3 version you can replace Shift jis characters drawing with non shift jis ones (useful for fan translation, where you can replace ぁ with é for example)\
![image](https://user-images.githubusercontent.com/69110695/193106532-50f10d21-9a71-4f35-bfd7-592a3f3bac3e.png)

