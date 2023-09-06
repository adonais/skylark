# skylark:
Skylark Editor is a customizable text/hex editor. Small, Portable, Fast.  
Compared with vs code, atom, and more editors, it can start up in less than a second~~  
  
# features:
* Built in file manager and SFTP remote manager.  
* Binary/Hex viewer for files of unlimited size.  
* Encryption and decryption of common key algorithms.  
* Perl Compatible Regular Expression.  
* Support for many encodings, Syntax highlight for a lot of languages.  
* Supports sumatrapdf plugin.  
* Supports clang-format.  
* Supports code snippet.  
* Supports dark mode. 

# Build:
* vc14 or above:
  ```
      nmake clean
      nmake
  ```
* clang:
  ```
      nmake CC=clang-cl clean
      nmake CC=clang-cl
  ```
# Screenshots:
* sumatrapdf plugin
![images/sumatrapdf-plugin.png](images/sumatrapdf-plugin.png)
* clang-format
![images/clang-format.png](images/clang-format.png)
* execute lua script
![images/lua-engine.png](images/lua-engine.png)
* execute sql
![images/sql_clinet.png](images/sql_client.png)
* json parsing
![images/json_search.png](images/json_search.png)
* hex view
![images/hex_view.png](images/hex_view.png)
* linux wine
![images/skylark_wine.png](images/skylark_wine.png)
