# Setuper
> [!CAUTION] 
> The code in this repo is more buggy than my mattress

### A little program setup and install script for MacOS, made purely from love!

## Building
### Prerequisites: 
1. lzfse library (the dylib works, just put it in the ```bin``` directory before running) 
2. libcurl (duh)
3. zlib and bzlib2
4. FUSE (macfuse works well enough for me)
5. hfsfuse (https://github.com/0x09/hfsfuse)

To build, just: 
```sh
git clone https://github.com/0wenches/setuper/
cd setuper
make
```
## Usage
The program uses FUSE, so it will require being run as root
```sh
cd bin/
sudo ./setuper
```
## Making your own program list
If you want to install different programs, other than the ones in the programlist, you can add your own entries to it. 
If you've already built the program, change the programs.csv file in the ```bin``` directory, otherwise change the one in ```src```. 
The list (currently) has only two variables per entry, the first one is used for the program installer name, the second being the link to the installer. 
**The installer links may not be working!**(duh)

## To Do list: 
- [x] Download the installers for programs
- [x] Convert DMGs to IMG files
- [ ] Extract bundles from IMG files
- [ ] Configs for the programs
