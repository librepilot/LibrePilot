# qt5printers
The `core.py`, `typeinfo.py` and `__init__.py` files are taken from patchset 2
at https://codereview.qt-project.org/87052 and provide a GDB pretty printer for
Qt5. These are authored by Alex Merry from the KDE project.

## Usage
Copy the three Python files to `~/.gdb/qt5printers/` and add this to your
`~/.gdbinit` (or execute it from an existing `gdb` session):


    python
    import sys, os.path
    sys.path.insert(0, os.path.expanduser('~/.gdb'))
    import qt5printers
    qt5printers.register_printers(gdb.current_objfile())
    end

Now verify it with your favorite program. Below you can find a quick test
program.

### Test program
Here is a test program (save it as `test.cpp`):

    #include <QTextStream>
    void test(const QByteArray & ba) { }
    int main(void) {
        test(QByteArray("abc"));
        return 0;
    }

Compile it with:

    g++ test.cpp $(pkg-config --cflags --libs Qt5Core) -g

If everything goes well you should see the expanded data:

    $ gdb -q -ex break\ test -ex r ./a.out
    ...
    Breakpoint 1, test (ba="abc" = {...}) at test.cpp:4
    4           test(QByteArray("abc"));

## Background
The Qt4 pretty printers from KDevelop[0] are not fully compatible with Qt5. For
instance, the latest version (from December 2014) does not properly handle
QByteArray. While these qt5printers are compatible with Qt5, it conflicts with
Qt4 (for example, QByteArray changed in Qt 5 from Qt 4 in this commit[1]).

See also:

 - https://bugs.kde.org/show_bug.cgi?id=331044
 - https://techbase.kde.org/Development/Tutorials/Debugging/Debugging_with_GDB


## License
For the applicable licenses, see the headers of the files and refer to the Qt5
sources at https://code.qt.io/cgit/qt/qtbase.git/tree/.

 [0]: https://projects.kde.org/projects/extragear/kdevelop/kdevelop/repository/revisions/master/show/debuggers/gdb/printers
 [1]: https://code.qt.io/cgit/qt/qtbase.git/commit/src/corelib/tools/qbytearray.h?id=ad35a41739c8e1fb6db62ed37b764448b2e59ece
