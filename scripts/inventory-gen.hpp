/* !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!! */

#include <iostream>
#include <map>
#include <list>

using dict = std::map<std::string,std::string>;
using dictOfDict = std::map<std::string,dict>;

/* Dictionary of Properties */
using propertiesDict = std::map<std::string,dictOfDict>;

/* Dictionary of Interface and the Properties */
using interfacePropertiesDict = std::map<std::string,dictOfDict>;

/* Dictionary of object and the interfaces */
using objectInterfaceDict = std::map<std::string,interfacePropertiesDict>;

/* Dictionary of fru and the objects */
using fruObjectDict = std::map<std::string,objectInterfaceDict>;

