# Sourcery

A macro engine and project generation tool for text source files.

Sourcery currently works with the following features outlined below:

## Features

1. Folder Generation

    The token `#!%` will attempt to create a folder at the location provided after
    the `%` symbol. The location will only support relative paths for now.

    ```#!%path/myfolder```

    A folder will be created at the invoking directory if it hasn't already.

2. File Generation w/ Text

    The token `#!+` will attempt to create a file at the location provided after
    the `+` symbol. Additionally, all text proceeding a colon will be append to
    that file as well.

    ```#!+test.txt:hello, world!```

    The above line will create the file text.txt at the invoking directory if it
    isn't already created. The text which proceeds the colon will be appended to
    the file.

	Multi-line text operations can use `<<(` and `)>>` identifiers to append more
	formatted text to the file being generated. In order for Sourcery to see this,
	the left `<<(` must appear on the same line as the directive. The following right
	`)>>` can appear at any pointer after, either on the same line or several lines
	down. Sourcery will ignore all directives until it finds a matching `)>>`.

	```
	#!+main.cpp:<<(#include <iostream>
	int main(int argc, char** argv)
	{
		std::cout << "Hello, world\n";
		return 0;
	}
	)>>
	```

	This will generate exactly as follows:
	```
	#include <iostream>
	int main(int argc, char** argv)
	{
		std::cout << "Hello, world\n";
		return 0;
	}
	```

3. Command-line Execution

	Sourcery can also execute commands on the command line. This can be achieved
	by with the token `#!!`. You can use this to perform CLI operations. These will
	block execution until they are complete as their order may be consequential.

	```#!!cmake -B build```

