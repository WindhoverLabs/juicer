# Table of Contents
1. [Dependencies](#dependencies)
2. [Environment Setup](#environment-setup)
3. [Testing](#testing)


## Dependencies <a name="dependencies"></a>
* `libdwarf`
* `libelf`
* `sqlite3`
* `C++14`
* `Catch2`
* `gcc  5.4.0`

# Environment Setup <a name="environment-setup"></a>

Most of this project is written in C++14, with some parts in C because of libraries like `sqlite3` and `libdwarf`.

* Eclipse CDT 9.7.0.201903092251
* C++14
* Ubuntu 16.04.6 LTS

We currently use Eclipse to maintain this project and we are trying to not make that a painful dependency that
developers have to deal with. This is why the Eclipse project is under `juicer/juicer`. We know it's not the best
solution, but we'll try to make this a little more modular in the future to be usable in other IDEs and environments. 
For now, if you want to get started on Juicer, your best bet is using Eclipse. Hopefully we'll find a more modular non-Eclipse way of doing this.

## Debugging in Eclipse

By default this version of Eclipse uses `gdb 7.11.1`, but this version does not have support for inspecting smart pointers. You need to setup `lldb`, llvm's debugger.

 
### Setting up lldb on  Eclipse

1. `sudo apt-get install lldb`
2. Now on Elcipse go to **Help->Install New Software...** and Install LLDB Debugger
![eclipse](Images/Cdt-lldb-install.png  "eclipse-debug")

3. Then right-click on your project and go to **Juicer->Debug As->Debug Configurations**
4. Click on the **select other...** option at the bottom of the window
![eclipse](Images/Lldb-set-delagate.png  "eclipse-debug")

5. Set `LLDB` as your default debugger

![eclipse](Images/Lldb-set-delegate2.png
  "eclipse-debug")
  
  And you are all set!
  
## Testing <a name="testing"></a>
We currently use the [Catch2](https://github.com/catchorg/Catch2) framework for our Unit testing. It is integrated into the repo as a submodule. You can find
all of our testing under the `Testing` folder. Just like Juicer, it is an Eclipse project. So, if you already use Eclipse, 
then just import the project and you can test away!

### Writing Unit Tests

`Catch2` allows to easily write very readable tests that are very easy to maintain and write. Below is an example of this
that tests the correctness of an ElfFile's name on `Testing/TestElfFile.cpp`:

	#include "catch.hpp"
	#include "ElfFile.h"
	
	TEST_CASE("Test name correctness", "[ElfFile]")
	{
	    ElfFile elffy{};
	    std::string elffyName{"elffy.o"};
	    elffy.setName(elffyName);
	
	    REQUIRE(elffy.getName() == elffyName);
	}
	
This test called "Test name correctness" will construct a very simple ElfFile object and check that when we set a name with `setName` we get the same exact name back when we call `getName`. The `REQUIRE` macro is the actual assertion that verifies this. If the test fails(meaning the `REQUIRE` macro is false), then our test and any other tests that follow stop executing. Catch2 has more interesting features like `CHECK` and syntax like `WHEN` for behavior-driven development that
you can read all about in the link above.
	
Note that we don't define a `main` function here so we define one very easily in `Testing/main.cpp`:

	/*This tells Catch to provide a main() - only do this in one cpp file*/
	#define CATCH_CONFIG_MAIN
	
	
	/**This disables coloring output so that Eclipse CDT(Version: 9.7.0.201903092251)
	 *will be able to render it. If you really like colored output, you'll have to use
	 *something else other than Eclipse's console(such as GNOME shell) to run the tests
	 *and comment out the CATCH_CONFIG_COLOUR_NONE macro.
	 */
	#define CATCH_CONFIG_COLOUR_NONE
	
	#include "catch.hpp"
	
Yes, that's it! Catch2 will read the `CATCH_CONFIG_MAIN` and generate a `main` function for you. The `CATCH_CONFIG_COLOUR_NONE` is not necessary to run Catch2, but if you run into problems where the output will not render properly because it is colored(like in Eclipse), the you might find this macro useful. 


Now all you gotta do is build your project on Eclipse(or from the terminal) and the all of your tests.

### Setting up gcov

To track our test coverage we use `gcov`. 

In order to set up gcove we need to pass the `--coverage` flag to gcc and then pass `lgcov` and  `--coverage` and compile and run your the tests.

### Generating Coverage Report
After you compile *and* run the test suite with gcov, generating an html report is very easy! All you need  is two commands.

*Make sure you are in the directory that has all of the `gcda ` and `gcno ` files*

1. Generate the raw report with `lcov`
```
lcov -c --directory . --output-file main_coverage.info
```

This scans the directory `.` for the `gcda ` and `gcno ` files and generates a non-human redable report(a raw report) and saves it to main_coverage.info file.

2. Generate a html report that humans can actually read

```
genhtml main_coverage.info --output-directory out 
```
This will create an html report that we can view on the browser and save it to the `out` directory.

