#include <Packages/Uintah/Core/ProblemSpec/ProblemSpec.h>
#include <Packages/Uintah/Core/Exceptions/ParameterNotFound.h>
#include <Core/Geometry/IntVector.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/Point.h>
#include <Core/XMLUtil/XMLUtil.h>
#include <Core/Malloc/Allocator.h>
#include <sgi_stl_warnings_off.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <sgi_stl_warnings_on.h>

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  define IRIX
#  pragma set woff 1375
#endif

#include <libxml/tree.h>

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma reset woff 1375
#endif

using namespace Uintah;
using namespace SCIRun;

using namespace std;

// Forward Declarations
//std::ostream& operator<<(std::ostream& out, const xmlNode & toWrite);
//std::ostream& operator<<(std::ostream& out, const DOMText & toWrite);

ProblemSpec::~ProblemSpec()
{
  // the problem spec doesn't allocate any new memory.
}

ProblemSpecP
ProblemSpec::findBlock() const
{
  const xmlNode* child = d_node->children;
  if (child != 0) {
    if (child->type == XML_TEXT_NODE) {
      child = child->next;
    }
  }
  if (child == NULL)
     return 0;
  else
     return scinew ProblemSpec(child, d_write);
}

ProblemSpecP 
ProblemSpec::findBlock(const string& name) const 
{
  if (d_node == 0)
    return 0;
  const xmlNode *child = d_node->children;
  while (child != 0) {
    string child_name(to_char_ptr(child->name));
    if (name == child_name) {
      return scinew ProblemSpec(child, d_write);
    }
    child = child->next;
  }
  return 0;
}

ProblemSpecP ProblemSpec::findNextBlock() const
{
  const xmlNode* found_node = d_node->next;
  
  if (found_node != 0) {
    if (found_node->type == XML_TEXT_NODE) {
      found_node = found_node->next;
    }
  }
    
  if (found_node == NULL ) {
     return 0;
  }
  else {
     return scinew ProblemSpec(found_node, d_write);
  }
}

ProblemSpecP
ProblemSpec::findNextBlock(const string& name) const 
{
  // Iterate through all of the child nodes of the next node
  // until one is found that has this name

  const xmlNode* found_node = d_node->next;

  while(found_node != 0){
    string c_name(to_char_ptr(found_node->name));
    if (c_name == name) {
      break;
    }

    found_node = found_node->next;
  }
  if (found_node == NULL) {
     return 0;
  }
  else {
     return scinew ProblemSpec(found_node, d_write);
  }
}

ProblemSpecP
ProblemSpec::findTextBlock()
{
   for (xmlNode* child = d_node->children; child != 0;
        child = child->next) {
     if (child->type == XML_TEXT_NODE) {
       return scinew ProblemSpec(child, d_write);
      }
   }
   return NULL;
}

string
ProblemSpec::getNodeName() const
{
  return string(to_char_ptr(d_node->name));
}

short
ProblemSpec::getNodeType() 
{
  return d_node->type;
}

ProblemSpecP
ProblemSpec::importNode(ProblemSpecP src, bool deep) 
{
  xmlNode* d = xmlDocCopyNode(src->d_node, d_node->doc, deep ? 1 : 0);
  if (d)
    return scinew ProblemSpec(d, d_write);
  else
    return 0;
}

void
ProblemSpec::replaceChild(ProblemSpecP toreplace, 
                          ProblemSpecP replaced) 
{
  xmlNode* d = xmlReplaceNode(toreplace->d_node, replaced->d_node);

  if (d)
    xmlFreeNode(d);
}

void
ProblemSpec::removeChild(ProblemSpecP child)
{
  xmlUnlinkNode(child->getNode());
  xmlFreeNode(child->getNode());
}

//______________________________________________________________________
//
void
checkForInputError(const string& stringValue, 
                   const string& Int_or_float)
{
  //__________________________________
  //  Make sure stringValue only contains valid characters
  if ("Int_or_float" != "int") {
    string validChars(" -+.0123456789eE");
    string::size_type  pos = stringValue.find_first_not_of(validChars);
    if (pos != string::npos){
      ostringstream warn;
      warn << "Input file error: I found ("<< stringValue[pos]
           << ") inside of "<< stringValue<< " at position "<< pos
           << "\nIf this is a valid number tell me --Todd "<<endl;
      throw ProblemSetupException(warn.str(), __FILE__, __LINE__);
    }
    //__________________________________
    // check for two or more "."
    string::size_type p1 = stringValue.find_first_of(".");    
    string::size_type p2 = stringValue.find_last_of(".");     
    if (p1 != p2){
      ostringstream warn;
      warn << "Input file error: I found two (..) "
           << "inside of "<< stringValue
           << "\nIf this is a valid number tell me --Todd "<<endl;
      throw ProblemSetupException(warn.str(), __FILE__, __LINE__);
    }
  }  
  if (Int_or_float == "int")  {
    string validChars(" -0123456789");
    string::size_type  pos = stringValue.find_first_not_of(validChars);
    if (pos != string::npos){
      ostringstream warn;
      warn << "Input file error Integer Number: I found ("<< stringValue[pos]
           << ") inside of "<< stringValue<< " at position "<< pos
           << "\nIf this is a valid number tell me --Todd "<<endl;
      throw ProblemSetupException(warn.str(), __FILE__, __LINE__);
    }
  }
} 

ProblemSpecP
ProblemSpec::get(const string& name, double &value)
{
  ProblemSpecP ps = this;

  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string stringValue = getNodeValue();
        checkForInputError(stringValue,"double"); 
        istringstream ss(stringValue);
        ss >> value;
      }
    }
  }
          
  return ps;
}

ProblemSpecP
ProblemSpec::get(const string& name, unsigned int &value)
{
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string stringValue = getNodeValue();
        checkForInputError(stringValue,"int");
        istringstream ss(stringValue);
        ss >> value;
      }
    }
  }
          
  return ps;

}

ProblemSpecP
ProblemSpec::get(const string& name, int &value)
{
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string stringValue = getNodeValue();
        checkForInputError(stringValue,"int");
        istringstream ss(stringValue);
        ss >> value;
      }
    }
  }
          
  return ps;

}


ProblemSpecP
ProblemSpec::get(const string& name, long &value)
{
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string stringValue = getNodeValue();
        checkForInputError(stringValue,"int");
        stringstream ss(stringValue);
        ss >> value;
      }
    }
  }
          
  return ps;

}

ProblemSpecP
ProblemSpec::get(const string& name, bool &value)
{
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string cmp = getNodeValue();
        // Slurp up any spaces that were put in before or after the cmp string.
        istringstream result_stream(cmp);
        string nospace_cmp;
        result_stream >> nospace_cmp;
        if (nospace_cmp == "false") {
         value = false;
        }
        else if  (nospace_cmp == "true") {
         value = true;
        } else {
         string error = name + "Must be either true or false";
         throw ProblemSetupException(error, __FILE__, __LINE__);
        }
      }
    }
  }
          
  return ps;

}

ProblemSpecP
ProblemSpec::get(const string& name, string &value)
{
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        //__________________________________
        // This little bit of magic removes all spaces
        string tmp = getNodeValue();
        istringstream value_tmp(tmp);
        value_tmp >> value; 
      }
    }
  }
          
  return ps;

}

ProblemSpecP
ProblemSpec::get(const string& name, 
                 Point &value)
{
    Vector v;
    ProblemSpecP ps = get(name, v);
    value = Point(v);
    return ps;
}

ProblemSpecP
ProblemSpec::get(const string& name, 
                 Vector &value)
{

  string string_value;
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string_value = getNodeValue();

        // Parse out the [num,num,num]
        // Now pull apart the string_value
        string::size_type i1 = string_value.find("[");
        string::size_type i2 = string_value.find_first_of(",");
        string::size_type i3 = string_value.find_last_of(",");
        string::size_type i4 = string_value.find("]");
        
        string x_val(string_value,i1+1,i2-i1-1);
        string y_val(string_value,i2+1,i3-i2-1);
        string z_val(string_value,i3+1,i4-i3-1);
       
        checkForInputError(x_val, "double"); 
        checkForInputError(y_val, "double");
        checkForInputError(z_val, "double");

        value.x(atof(x_val.c_str()));
        value.y(atof(y_val.c_str()));
        value.z(atof(z_val.c_str()));   
      }
    }
  }
          
  return ps;

}

// value should probably be empty before calling this...
ProblemSpecP
ProblemSpec::get(const string& name, 
                 vector<double>& value)
{
  vector<string> string_values;
  if(!this->get(name, string_values)) {
    return 0;
  }
  
  for(vector<string>::const_iterator vit(string_values.begin());
      vit!=string_values.end();vit++) {
    const string v(*vit);
    
    checkForInputError(v, "double"); 
    value.push_back( atof(v.c_str()) );
  }
  
  return this;
}

// value should probably be empty before calling this...
ProblemSpecP
ProblemSpec::get(const string& name, 
                 vector<int>& value)
{
  vector<string> string_values;
  if(!this->get(name, string_values)) {
    return 0;
  }
  
  for(vector<string>::const_iterator vit(string_values.begin());
      vit!=string_values.end();vit++) {
    const string v(*vit);
    
    checkForInputError(v, "int"); 
    value.push_back( atoi(v.c_str()) );
  }
  
  return this;
} 

// value should probably be empty before calling this...
ProblemSpecP
ProblemSpec::get(const string& name, 
                 vector<string>& value)
{

  string string_value;
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string_value = getNodeValue();
        
        istringstream in(string_value);
        char c,next;
        string result;
        while (!in.eof()) {
         in >> c;
         if (c == '[' || c == ',' || c == ' ' || c == ']')
           continue;
         next = in.peek();
         result += c;
         if (next == ',' ||  next == ' ' || next == ']') {
           // push next string onto stack
           value.push_back(result);
           result.erase();
         }
        }
      }
    }
  }
  
  return ps;
} 

ProblemSpecP
ProblemSpec::get(const string& name, 
                 IntVector &value)
{

  string string_value;
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string_value = getNodeValue();
        parseIntVector(string_value, value);
      }
    }
  }
          
  return ps;

}

ProblemSpecP
ProblemSpec::get(const string& name, vector<IntVector>& value)
{
  string string_value;
  ProblemSpecP ps = this;
  ProblemSpecP node = findBlock(name);
  if (node == 0) {
    ps = 0;
    return ps;
  }
  else {
    xmlNode* found_node = node->d_node;
    for (xmlNode* child = found_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        string_value = getNodeValue();

        istringstream in(string_value);
        char c;
        bool first_bracket = false;
        bool inner_bracket = false;
        string result;
        // what we're going to do is look for the first [ then pass it.
        // then if we find another [, make a string out of it until we see ],
        // then pass that into parseIntVector, and repeat.
        while (!in.eof()) {
          in >> c;
          if (c == ' ' || (c == ',' && !inner_bracket))
            continue;
          if (c == '[') {
            if (!first_bracket) {
              first_bracket = true;
              continue;
            }
            else {
              inner_bracket = true;
            }
          }
          else if (c == ']') {
            if (inner_bracket) {
              // parse the string for an IntVector
              IntVector val;
              result += c;
              // it should be [num,num,num] by now
              parseIntVector(result, val);
              value.push_back(val);
              result.erase();
              inner_bracket = false;
              continue;
            }
            else
              break; // end parsing on outer ]
          }
          // add the char to the string
          result += c;
        }  // end while (!in.eof())
      } // end if (child->type == XML_TEXT_NODE)
    }
  }
  return ps;
}

void ProblemSpec::parseIntVector(const string& string_value, IntVector& value)
{
  // Parse out the [num,num,num]
  // Now pull apart the string_value
  string::size_type i1 = string_value.find("[");
  string::size_type i2 = string_value.find_first_of(",");
  string::size_type i3 = string_value.find_last_of(",");
  string::size_type i4 = string_value.find("]");
  
  string x_val(string_value,i1+1,i2-i1-1);
  string y_val(string_value,i2+1,i3-i2-1);
  string z_val(string_value,i3+1,i4-i3-1);

  checkForInputError(x_val, "int");     
  checkForInputError(y_val, "int");     
  checkForInputError(z_val, "int");     
          
  value.x(atoi(x_val.c_str()));
  value.y(atoi(y_val.c_str()));
  value.z(atoi(z_val.c_str())); 

}

bool ProblemSpec::get(int &value)
{
   for (xmlNode *child = d_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
         value = atoi(getNodeValue().c_str());
         return true;
      }
   }
   return false;
}

bool ProblemSpec::get(long &value)
{
   for (xmlNode *child = d_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
         value = atoi(getNodeValue().c_str());
         return true;
      }
   }
   return false;
}

bool ProblemSpec::get(double &value)
{
   for (xmlNode *child = d_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
         value = atof(getNodeValue().c_str());
         return true;
      }
   }
   return false;
}

bool ProblemSpec::get(string &value)
{
   for (xmlNode *child = d_node->children; child != 0;
        child = child->next) {
      if (child->type == XML_TEXT_NODE) {
        // Remove the white space from the front and back of the string
        string tmp(getNodeValue());
        istringstream tmp_str(tmp);
        string w;
        while(tmp_str>>w) value += w;
        return true;
      }
   }
   return false;
}

bool ProblemSpec::get(Vector &value)
{
  string string_value;
  for (xmlNode* child = d_node->children; child != 0;
      child = child->next) {
    if (child->type == XML_TEXT_NODE) {
     string_value = getNodeValue();
     // Parse out the [num,num,num]
     // Now pull apart the string_value
     string::size_type i1 = string_value.find("[");
     string::size_type i2 = string_value.find_first_of(",");
     string::size_type i3 = string_value.find_last_of(",");
     string::size_type i4 = string_value.find("]");

     string x_val(string_value,i1+1,i2-i1-1);
     string y_val(string_value,i2+1,i3-i2-1);
     string z_val(string_value,i3+1,i4-i3-1);

     checkForInputError(x_val, "double"); 
     checkForInputError(y_val, "double");
     checkForInputError(z_val, "double");

     value.x(atof(x_val.c_str()));
     value.y(atof(y_val.c_str()));
     value.z(atof(z_val.c_str()));      
    }
  }        
  return false;
}


ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            double& value, double defaultVal) 
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value = defaultVal;
  }

  return ps;
}
ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            int& value, int defaultVal)
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value=defaultVal;
  }

  return ps;
}
ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            bool& value, bool defaultVal)
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value=defaultVal;
  }

  return ps;
}
ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            string& value, 
                            const string& defaultVal)
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value = defaultVal;
  }
  return ps;
}
ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            IntVector& value, 
                            const IntVector& defaultVal)
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value = defaultVal;
  }

  return ps;
}

ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            Vector& value, 
                            const Vector& defaultVal)
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value = defaultVal;
  }

  return ps;
}

ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            Point& value, 
                            const Point& defaultVal)
{
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;
    value = defaultVal;
  }

  return ps;
}

ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            vector<double>& value, 
                            const vector<double>& defaultVal)
{
  value.clear();
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    //create xmlNode to add to the tree
    appendElement(name.c_str(), defaultVal);

    // set default values
    ps = this;

    value.clear();
    int size = static_cast<int>(defaultVal.size());
    for (int i = 0; i < size; i++)
      value.push_back(defaultVal[i]);
  }

  return ps;
}

ProblemSpecP
ProblemSpec::getWithDefault(const string& name, 
                            vector<int>& value, 
                            const vector<int>& defaultVal)
{
  value.clear();
  ProblemSpecP ps = get(name, value);
  if (ps == 0) {

    // add xmlNode to the tree
    appendElement(name.c_str(), defaultVal);
    // set default values
    ps = this;
    value.clear();
    int size = static_cast<int>(defaultVal.size());
    for (int i = 0; i < size; i++)
      value.push_back(defaultVal[i]);
  }

  return ps;
}

// add newline before, then tabs tabs, and then if trail is true, ending whitespace
void ProblemSpec::appendElement(const char* name, const string& value,
                                bool trail /*=0*/, int tabs /*=1*/) 
{
  ostringstream ostr;
  ostr.clear();
  ostr << "\n";
  for (int i = 0; i < tabs; i++)
    ostr << "\t";

  appendText(ostr.str().c_str());
  
  xmlNewChild(d_node, 0, BAD_CAST name, BAD_CAST value.c_str());
  
  if (trail) {
    appendText("\n");
  }
}

//basically to make sure correct overloaded function is called
void ProblemSpec::appendElement(const char* name, const char* value,
                                bool trail /*=0*/, int tabs /*=1*/) {
  appendElement(name, string(value), trail, tabs);
}


void ProblemSpec::appendElement(const char* name, int value,
                                bool trail /*=0*/, int tabs /*=1*/) 
{
   ostringstream val;
   val << value;
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, long value,
                                bool trail /*=0*/, int tabs /*=1*/) 
{
   ostringstream val;
   val << value;
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, const IntVector& value,
                                bool trail /*=0*/, int tabs /*=1*/) 
{
   ostringstream val;
   val << '[' << value.x() << ", " << value.y() << ", " << value.z() << ']';
   appendElement(name, val.str(), trail, tabs);
}

void ProblemSpec::appendElement(const char* name, const Point& value,
                                bool trail /*=0*/, int tabs /*=1*/) 
{
   ostringstream val;
   val << '[' << setprecision(17) << value.x() << ", " << setprecision(17) << value.y() << ", " << setprecision(17) << value.z() << ']';
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, const Vector& value,
                                bool trail /*=0*/, int tabs /*=1*/)
{
   ostringstream val;
   val << '[' << setprecision(17) << value.x() << ", " << setprecision(17) << value.y() << ", " << setprecision(17) << value.z() << ']';
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, double value,
                                bool trail /*=0*/, int tabs /*=1*/)
{
   ostringstream val;
   val << setprecision(17) << value;
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, const vector<double>& value,
                                bool trail /*=0*/, int tabs /*=1*/)
{
   ostringstream val;
   val << '[';
   for (unsigned int i = 0; i < value.size(); i++) {
     val << setprecision(17) << value[i];
     if (i !=  value.size()-1)
       val << ',';
     
   }
   val << ']';
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, const vector<int>& value,
                                bool trail /*=0*/, int tabs /*=1*/)
{
   ostringstream val;
   val << '[';
   for (unsigned int i = 0; i < value.size(); i++) {
     val << setprecision(17) << value[i];
     if (i !=  value.size()-1)
       val << ',';
     
   }
   val << ']';
   appendElement(name, val.str(), trail, tabs);

}

void ProblemSpec::appendElement(const char* name, bool value,
                                bool trail /*=0*/, int tabs /*=1*/)
{
  if (value)
    appendElement(name, string("true"), trail, tabs);
  else
    appendElement(name, string("false"), trail, tabs);
}

void ProblemSpec::require(const string& name, double& value)
{

  // Check if the prob_spec is NULL

  if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);
 
}

void ProblemSpec::require(const string& name, int& value)
{

 // Check if the prob_spec is NULL

  if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);
  
}

void ProblemSpec::require(const string& name, unsigned int& value)
{

 // Check if the prob_spec is NULL

  if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);
  
}

void ProblemSpec::require(const string& name, long& value)
{

 // Check if the prob_spec is NULL

  if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);
  
}

void ProblemSpec::require(const string& name, bool& value)
{
 // Check if the prob_spec is NULL

  if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);
 

}

void ProblemSpec::require(const string& name, string& value)
{
 // Check if the prob_spec is NULL

  if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);
 
}

void ProblemSpec::require(const string& name, 
                       Vector  &value)
{

  // Check if the prob_spec is NULL

 if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);

}

void ProblemSpec::require(const string& name, 
                       vector<double>& value)
{

  // Check if the prob_spec is NULL

 if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);

}

void ProblemSpec::require(const string& name, 
                       vector<int>& value)
{

  // Check if the prob_spec is NULL

 if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);

} 

void ProblemSpec::require(const string& name, 
                       vector<IntVector>& value)
{

  // Check if the prob_spec is NULL

 if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);

} 

void ProblemSpec::require(const string& name, 
                       IntVector  &value)
{

  // Check if the prob_spec is NULL

 if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);

}

void ProblemSpec::require(const string& name, 
                       Point  &value)
{
 // Check if the prob_spec is NULL
 if (! this->get(name,value))
      throw ParameterNotFound(name, __FILE__, __LINE__);

}

void
ProblemSpec::getAttributes(map<string,string>& attributes)
{
  attributes.clear();

  xmlAttr* attr = d_node->properties;

  for (; attr != 0; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE) {
      attributes[to_char_ptr(attr->name)] = to_char_ptr(attr->children->content);
    }
  }
}

bool
ProblemSpec::getAttribute(const string& attribute, string& result)
{

  map<string, string> attributes;
  getAttributes(attributes);

  map<string,string>::iterator iter = attributes.find(attribute);

  if (iter != attributes.end()) {
    result = iter->second;
    return true;
  }
  else {
    return false;
  }
}

void
ProblemSpec::setAttribute(const string& name, 
                          const string& value)
{
  xmlNewProp(d_node, BAD_CAST name.c_str(), BAD_CAST value.c_str());
}


ProblemSpecP
ProblemSpec::getFirstChild() 
{
  xmlNode* d = d_node->children;
  if (d)
    return scinew ProblemSpec(d, d_write);
  else
    return 0;
}

ProblemSpecP
ProblemSpec::getNextSibling() 
{
  xmlNode* d = d_node->next;
  if (d)
    return scinew ProblemSpec(d, d_write);
  else
    return 0;
}

string
ProblemSpec::getNodeValue() 
{
  string ret(to_char_ptr(d_node->children->content));
  return ret;
}

void
ProblemSpec::appendText(const char* str)
{
  xmlNewTextChild(d_node, 0, BAD_CAST "name", BAD_CAST str);
}

// append element with associated string
// preceded by \n (if lead is true) with tabs tabs (default 0), and followed by a newline
ProblemSpecP
ProblemSpec::appendChild(const char *str, 
                         bool lead /*=0*/, int tabs /*=0*/) 
{
  ostringstream ostr;
  ostr.clear();
  if (lead)
    ostr << "\n";
  for (int i = 0; i < tabs; i++)
    ostr << "\t";

  appendText(ostr.str().c_str());
  
  xmlNode* elt = xmlNewChild(d_node, 0, BAD_CAST str, 0);
  
  return scinew ProblemSpec(elt, d_write);
}

void
ProblemSpec::appendChild(ProblemSpecP pspec) 
{
  xmlAddChild(d_node, pspec->d_node);
}

void
ProblemSpec::addStylesheet(char* type, char* name) 
{
#if 0
   ASSERT((strcmp(type, "css") == 0) || strcmp(type, "xsl") == 0);

   string str1 = "xml-stylesheet";

   ostringstream str;
   str << " type=\"text/" << type << "\" href=\"" << name << "\"";
   string str2 = str.str().c_str();

   xmlAddPrevSibling(xmlDocGetRootElement(d_node->doc),
                     xmlNewDocPI(d_node->doc, BAD_CAST str1.c_str(), BAD_CAST str2.c_str()));
#endif
}

// filename is a default param (NULL)
//   call with no parameters or NULL to output
//   to stdout
void
ProblemSpec::output(char* filename) const 
{
  if (filename) {
    xmlSaveFormatFileEnc(filename, d_node->doc, "UTF-8", 1);
  }
}

void
ProblemSpec::releaseDocument() 
{
  xmlFreeDoc(d_node->doc);
}

ProblemSpecP
ProblemSpec::getRootNode()
{
  xmlNode* root_node = xmlDocGetRootElement(d_node->doc);
  return scinew ProblemSpec(root_node,d_write);
}

const Uintah::TypeDescription*
ProblemSpec::getTypeDescription()
{
  //cerr << "ProblemSpec::getTypeDescription() not done\n";
  return 0;
}

// static
ProblemSpecP
ProblemSpec::createDocument(const string& name) 
{
  xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNodePtr node = xmlNewDocRawNode(doc, 0, BAD_CAST name.c_str(), 0);

  xmlDocSetRootElement(doc, node);

  return scinew ProblemSpec(node);
}

#if 0

void
outputContent(ostream& target, const char *chars /**to_write*/)
{
  //const char* chars = strdup(to_char_ptr(to_write));
  unsigned int len = chars?(unsigned)strlen(chars):0;
  for (unsigned int index = 0; index < len; index++) {
    switch (chars[index]) {
    case chAmpersand :
      target << "&amp;";
      break;

    case chOpenAngle :
      target << "&lt;";
      break;

    case chCloseAngle:
      target << "&gt;";
      break;

    case chDoubleQuote :
      target << "&quot;";
      break;

    default:
      // If it is none of the special characters, print it as such
      target << chars[index];
      break;
    }
  }
  delete[] chars;
}

namespace Uintah {
  ostream&
  operator<<(ostream& out, ProblemSpecP pspec)
  {
    out << *(pspec->getNode()->doc);
    return out;
  }
  
}


ostream&
operator<<(ostream& target, const xmlNode& toWrite) 
{
  // Get the name and value out for convenience
  const char *nodeName = XMLString::transcode(toWrite.name);
  const char *nodeValue = XMLString::transcode(toWrite.children->content);

  // nodeValue will be sometimes be deleted in outputContent, but
  // will not always call outputContent
  bool valueDeleted = false;

  switch (toWrite.type) {
  case XML_TEXT_NODE:
    {
      outputContent(target, nodeValue);
      valueDeleted = true;
      break;
    }

  case XML_PI_NODE :
    {
      target  << "<?"
              << nodeName
              << ' '
              << nodeValue
              << "?>";
      break;
    }

  case XML_DOCUMENT_NODE :
    {
      // Bug here:  we need to find a way to get the encoding name
      //   for the default code page on the system where the
      //   program is running, and plug that in for the encoding
      //   name.
      //MLCh *enc_name = XMLPlatformUtils::fgTransService->getEncodingName();
      target << "<?xml version='1.0' encoding='ISO-8859-1' ?>\n";

      xmlNode *brother = toWrite.next;
      while(brother != 0)
        {
          target << *brother << endl;
          brother = brother->next;
        }

      xmlNode *child = toWrite.children;
      while(child != 0)
        {
          target << *child << endl;
          child = child->next;
        }

      break;
    }

  case XML_ELEMENT_NODE :
    {
      // Output the element start tag.
      target << '<' << nodeName;

      // Output any attributes on this element
      DOMNamedNodeMap *attributes = toWrite.getAttributes();
      int attrCount = static_cast<int>(attributes->getLength());
      for (int i = 0; i < attrCount; i++) {
        xmlNode  *attribute = attributes->item(i);
        char* attrName = XMLString::transcode(attribute->name);
        target  << ' ' << attrName
                << " = \"";
        //  Note that "<" must be escaped in attribute values.
        outputContent(target, XMLString::transcode(attribute->getNodeValue(\
                                                                           )));
        target << '"';
        delete [] attrName;
      }

      //  Test for the presence of children, which includes both
      //  text content and nested elements.
      xmlNode *child = toWrite.children;
      if (child != 0) {
        // There are children. Close start-tag, and output children.
        target << ">";
        while(child != 0) {
          target << *child;
          child = child->next;
        }

        // Done with children.  Output the end tag.
        target << "</" << nodeName << ">";
      } else {
        //  There were no children.  Output the short form close of the
        //  element start tag, making it an empty-element tag.
        target << "/>";
      }
      break;
    }

  case XML_ENTITY_REF_NODE:
    {
      xmlNode *child;
      for (child = toWrite.children; child != 0;
           child = child->next)
        target << *child;
      break;
    }

  case XML_CDATA_SECTION_NODE:
    {
      target << "<![CDATA[" << nodeValue << "]]>";
      break;
    }

  case XML_COMMENT_NODE:
    {
      target << "<!--" << nodeValue << "-->";
      break;
    }

  default:
    cerr << "Unrecognized node type = "
         << (long)toWrite.type << endl;
  }

  delete [] nodeName;
  if (!valueDeleted)
    delete [] nodeValue;
  return target;
}

ostream&
operator<<(ostream& target, const DOMText& toWrite) {
  const char *p = XMLString::transcode(toWrite.getData());
  target << p;
  delete [] p;
  return target;
}

#endif
