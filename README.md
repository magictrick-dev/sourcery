# Sourcery

A macro engine and project generation tool for text source files.

## Development Notes

Since this project is being actively developed, Sourcery isn't ready to be used.
The current goal of the project is to prepare Sourcery for basic text-parsing and
identifying directives. Initial behaviors include generating folders, appending
text to files (either append or create), comments, script-mode, and source caching.

1. Folder Generation

    The token `#!%` will attempt to create a folder at the location provided after
    the `%` symbol. The location will only support relative paths for now.

    ```#!%path/myfolder```

    A folder will be created at the invoking directory if it hasn't already.

2. File Generation w/ Text

    The token `#!+` will attempt to create a file at the location provided after
    the `+` symbol. Additionally, all text proceeding a colon will be append to
    that file as well.

    ```#!+test.txt:hello, world!\n```

    The above line will create the file text.txt at the invoking directory if it
    isn't already created. The text which proceeds the colon will be appended to
    the file.

3. Comments

    Comments are pretty self-explanatory. The token for a comment is `##`.

4. Script Mode

    An important feature that allows the macro-processor to parse out the contents
    of a text file without stripping out the directives once it is complete. This
    is particularly handy for projects using a template file.

    ```#!!MODE=SCRIPT```

    This must appear somewhere in the file in order for script mode to be enabled.
    It's a good idea to have this placed at the top.

5. Source Caching

    If you run Sourcery on a text source and find that the results are not what
    you intended or that you intended the text source to run in script mode, then
    you may want to restore the original file. Source caching will be the method
    to restore text files to their prior state.

    This will be done through a CLI argument:
    
    ```sourcery --undo```

    Currently, undo will undo all changes made since the last invocation. In the
    future, selective undo will be added.

    Undo will not undo file/folder changes as of right now. The primary focus will
    be restoring the source text to its original state and any external modifications
    will not be restored. This will be changed in the future.
