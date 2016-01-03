This quick'n dirty tool will convert Hercules Interfaces to C++ Classes
+ Fixes method / member declarations.

See messdb.txt for an example definition.

The corresponding assignment was taken from .._defaults() method of the 
corresponding subsystem/interface.

If you're converting a 'new' interface - you'll have to manually add the 
Global Singleton (In Header as well as in source)
+ You'll have to remove the struct *_interface *...; declaration in the 
corresponding source file.


You may also have to add all Properties of the class as static + add them to the
corresponding Source file.


