# Page Size Configuration

The page size (currently set to `4096` bytes) is hardcoded as default template parameters across several data structures in the codebase. To change the page size globally, you should update the value in the following files:

- `omap/odsl/adaptive_oram.hpp`: Lines ~21, 22
- `omap/odsl/page_oram.hpp`: Lines ~11, 26
- `omap/odsl/circuit_oram.hpp`: Line ~31
- `omap/odsl/heap_tree.hpp`: Line ~24
- `omap/odsl/omap.hpp`: Line ~1262

*Note: In components like `ExtEMVector` and `NonCachedVector`, the page size is calculated dynamically and doesn't require hardcoding.*
