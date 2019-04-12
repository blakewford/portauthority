using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

public partial class Monitor
{
    const string DefaultDescription = @"
Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Nulla eu porta ante. Ut augue libero, aliquet a justo vel,
venenatis interdum enim. Vivamus et lorem pretium, congue quam sed,
commodo nulla. Cras finibus accumsan eros sit amet rhoncus.
Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Praesent ut elementum nisi. Vestibulum tristique arcu velit,
vitae tristique nisi volutpat vitae.
Phasellus semper nunc sapien, molestie sodales quam condimentum eget.
Quisque ultricies tempus blandit. Vestibulum laoreet erat a erat fermentum tincidunt.
Vestibulum a iaculis nulla. Mauris accumsan, nisl vitae ullamcorper mollis, massa nibh congue quam,
hendrerit ultrices velit urna eget risus. Etiam iaculis arcu elit,
non fringilla quam tristique sed. Fusce eleifend ullamcorper diam sit amet fringilla.
Donec urna nibh, hendrerit eget dictum vitae, feugiat et neque.
Aenean consequat maximus velit, a vehicula tellus sollicitudin nec.";

    const string BranchDescription = @"
A high percentage of branch instructions in your program may limit
the compiler's ability to pipeline the application.

This can result in unexpected stall conditions while the processor
executes instructions in strict series.

If possible consider alternatives such as bitwise operations,
conditional moves or if available branch predication";

    const string ControlDescription = @"
The control category is often associated with non-deterministic operations.
Examples include wait, break or entry points for architeture specific feature sets
that distract from the linear progression of compiled code.
The nop operation is also included in this group.

Generally speaking these operations should be avoided as they
can produce undesirable amounts of jitter in a system.";

    const string StackDescription = @"
Register focused operation.
Usually indicates positive progress through a given program.

Some stack instructions on CISC platfroms may function as macros to assist in the
implementation of higher level languages. In this instance, stack instructions
may become undesirable.";

    const string DataMovDescription = @"
Moving data stands in direct contrast to operating on data.
Best to limit the amount of time your program spends preparing
content to be used in computation.

For architectures with general purpose registers smaller than common
data types, like int32_t, spending time marshalling data may be unavoidable.";


}
