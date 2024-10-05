# WordplayQt

The original [wordplay](https://launchpad.net/ubuntu/+source/wordplay) is a command-line tool that generates anagrams from English words.  
`wordplay-qt` is a rewrite of that tool with both command-line and graphical user interfaces.

## Features

* Fast anagram finding algorithms
* Filtering results by
  * a specific word
  * specific characters
  * word length
* Generating rephrased anagrams
* Listing candidate words for the input word
* Reading custom wordlist files
* Simple GUI
* Support for multiple languages
* Sorting the results
* Writing the results to a file

## Usage

Basic command-line usage: `wordplay-qt [options] input_word`  

| Option                               | Description                                                                                  |
|--------------------------------------|----------------------------------------------------------------------------------------------|
| `-h`, `--help`                       | Displays help information.                                                                   |
| `-v`, `--version`                    | Displays version information.                                                                |
| `-a`, `--allow-multiple`             | Allow multiple occurrences of a word in an anagram.                                          |
| `-c`, `--characters <letters>`       | \<letters\> to include in words (useful with `--list` option).                               |
| `-d`, `--depth <amount>`             | Limit anagrams to \<amount\> of words.                                                       |
| `-f`, `--file <file>`                | \<file\> to use as word list ("-" for stdin).                                                |
| `-i`, `--include`                    | Include the input word in anagram list.                                                      |
| `-l`, `--list`                       | List candidate words.                                                                        |
| `-m`, `--max`, `--maximum <maximum>` | Candidate words must have a \<maximum\> number of characters.                                |
| `-n`, `--min`, `--minimum <minimum>` | Candidate words must have a \<minimum\> number of characters.                                |
| `-o`, `--output <file>`              | \<file\> to use for writing results (can be used with `--silent` and `--sort` options).      |
| `-p`, `--print`                      | Print results immediately (ignores `--sort` option).                                         |
| `-r`, `--rephrase`                   | Allow results to contain the same words in different order.                                  |
| `-s`, `--silent <level>`             | Silence \<level\> (1 = no info messages, 2 = no line numbers, 3 = no stats, 4 = no results). |
| `-w`, `--word <word>`                | \<word\> to include in anagrams (assumes i option).                                          |
| `-x`, `--no-generate`                | Do not generate anagrams (useful with `--list` option).                                      |
| `-z`, `--sort`                       | Sort the final results.                                                                      |

More details about the options can be displayed with `wordplay-qt --help` command.

## Examples

The input word is a positional argument that can be placed anywhere in the argument list. To have an input with spaces, you need to wrap it with quotes (`wordplay-qt "minimal is tic"` for example).  
You can omit the equals sign when using an option with two dashes (`--silent 3`), and the space can be omitted with single dash options (`-s3`).  
Single dash options can't be combined into one argument and should remain separate (change `-s3n3m10` to `-s3 -n3 -m10`).

> ```shell
> wordplay-qt minimalistic
> ```
> Anagram the string "minimalistic".

> ```shell
> wordplay-qt --sort minimalistic
> ```
> Sort and print anagrams of "minimalistic". By default, the anagram algorithm outputs the results in the order they are completed.

> ```shell
> wordplay-qt --list --no-generate minimalistic
> ```
> Print only the candidate words from the default wordlist that can be spelled by using the "minimalistic".

> ```shell
> wordplay-qt --minimum=3 --maximum=8 minimalistic
> ```
> Anagram the string "minimalistic". Do not use words shorter than 3 letters or longer than 8 letters.

> ```shell
> wordplay-qt --list --depth=3 --maximum=10 --file=/usr/share/dict/words minimalistic
> ```
> Print the candidate words for the string "minimalistic". Print anagrams containing up to 3 words and excluding words longer than 10 characters. Use the file "/usr/share/dict/words" rather than the default.

> ```shell
> wordplay-qt --minimum=3 --word=mimic --file=words.txt minimalistic
> ```
> Print anagrams of "minimalistic" containing the word "mimic" and use the file "words.txt" as the wordlist file. Ignore candidate words shorter than 3 characters.

> ```shell
> wordplay-qt --list --no-generate --characters=m minimalistic
> ```
> Print only the candidate words of "minimalistic" that contain the character "m".

> ```shell
> wordplay-qt --silent=3 --list --no-generate minimalistic
> ```
> Print only the candidate words for the string "minimalistic". The output will be just the words. This is useful for piping the output to another process or saving it to a file.

> ```shell
> wordplay-qt --silent=3 minimalistic
> ```
> Anagram the string "minimalistic" and print the anagrams with no line numbers. This is useful for piping the output to another process or saving it to a file.

> ```shell
> cat wordlist1 wordlist2 wordlist3 | sort -u | wordplay-qt --file=- minimalistic
> ```
> Anagram "minimalistic" by reading the data from stdin, so that the three wordlist files are combined and piped into `wordplay-qt`. The `sort -u` removes duplicate words from the combined wordlist.

## Compiling

Make sure the following are installed:
* `qt6-base`
* `cmake`
* `ninja`

### Compiling with Qt Creator

Make sure `qtcreator` is installed and follow these steps to build the project:
1. Open the CMakeLists.txt in Qt Creator
2. Configure the project
   * **Release** or **Debug** should be enough
3. Use the **Build** button/menu to begin compiling
   * Using **Run** option also works

The compiled `wordplay-qt` will be inside the configured "shadow build" folder (usually under the `build` folder).

### Compiling with command-line

You can get started with the following commands:
```shell
git clone --depth=1 https://github.com/Cofeiini/wordplay-qt.git
cd wordplay-qt
cmake -G Ninja -S . -B build
cmake --build build --parallel
```
After the compile process is done, `wordplay-qt` can be found in the `build` folder inside the source root.  
The CMake commands are the important parts as you need to clone the repository only once.

## Translation

This project supports an arbitrary amount of languages/regions and expands the localization options automatically when new IETF tags are added.

### User interface

These instructions apply to both the CLI and the GUI at the same time, but certain changes might be only reflected in one of them.

Follow these steps to add more languages:
1. Append the desired IETF tag to the `I18N_TRANSLATED_LANGUAGES` in [CMakeLists.txt](./CMakeLists.txt)
2. Reload the CMake project
   * This will generate an empty .ts file inside the [localization](./localization) folder
3. [Compile](#compiling) the project to populate the .ts file with values
   * Optionally you can run the CMake target `update_translations`
4. Use [Qt Linguist](https://doc.qt.io/qt-6/linguist-translators.html) tool to open the .ts file and proceed to [translate the text](https://doc.qt.io/qt-6/linguist-translating-strings.html)
5. Test the results
   1. [Compile](#compiling) the project
      * This will embed the new translations into the executable
   2. Run this tool
   3. Select the new language option
      * Remember to check the tooltips

> With the command-line you need to specify your language in the `LANGUAGE` environment variable before running this tool.  
> For example `LANGUAGE=fi-FI wordplay-qt --help` would print the help information in Finnish.  
> > On Windows defining temporary environment variables is not as easy and might require setting the values separately.

### Wordlist files

This project supports reading an arbitrary amount of wordlist files.

The files will be loaded in the following priority:
1. The local "words" folder (in the same location as the executable)
2. The config "words" folder (different for every platform; check below for the [usual paths](#application-config-path))
3. The embedded "en-US.txt" (should be overwritten by the other options)

> Higher priority file will override lower ones. 

#### Adding new files

To get started, simply generate a file where the words are separated by newline.
* Make sure the file contains no duplicates
  * This improves the performance as the tool needs to spend less time parsing the file
* The words do not need to be sorted, but it helps with debugging

##### With compiling

This option is for making development and testing easier. The process is simpler [without compiling](#without-compiling).

1. Move the file into the "words" folder under the "src" folder
   * This must be a .txt file with preferably IETF tag as the name
2. [Compile](#compiling) the project
3. Run this tool

##### Without compiling

1. Copy the file to the "words" folder under the [application config path](#application-config-path)
   * This must be a .txt file with preferably IETF tag as the name
2. Run this tool

> With the command-line you can specify the filename for `--file` (`--file="en-US.txt"` for example).  
> The tool will search for the file based on the [loading priority](#wordlist-files).

#### Application config path

This tool will attempt to generate a config folder during the startup process.

This config folder should be in one of the following locations:
* Under Linux, the path should be "**~/.config/Cofeiini/WordplayQt**"
* Under Windows, the path should be "**C:/Users/\<USER\>/AppData/Local/Cofeiini/WordplayQt**"
* Under macOS, the path should be "**~/Library/Preferences/Cofeiini/WordplayQt**"
