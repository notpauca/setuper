# Setuper
## Note: the code here isn't production-ready. And I don't give a **** if you've bricked your computer while using the code from this repo. 

### A little program setup and install script for MacOS, made purely from love!

## Building
To build, just: 
```sh
$ git clone https://github.com/0wenches/setuper/
$ cd setuper
$ make
```
## Usage
```sh
$ cd bin/
$ ./setuper
```
## Making your own program list
If you want to install different programs, other than the ones in the programlist, you can add your own entries to it. 
If you've already built the program, change the programs.csv file in the ```bin``` directory, otherwise change the one in ```src```. 
The list (currently) has only two sections per line, the first one is used for the program installer name, the second being the link to the installer. 
**The installer links may not be working!**

## To Do list: 
- [x] Download the installers for programs
- [x] Convert DMGs to IMG files
- [ ] Extract bundles from IMG files
- [ ] Configs for the programs
