/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


/*
 *  FrameworkPropertiesBase.h:  get and set CCA framework properties
 *
 *  Written by:
 *   Ayla Khan
 *   Scientific Computing and Imaging Institute
 *   University of Utah
 *   September 2004
 *
 *   Copyright (C) 2004 SCI Institute
 */


#ifndef SCIRun_FrameworkPropertiesBase_h
#define SCIRun_FrameworkPropertiesBase_h

namespace SCIRun {

  using namespace sci::cca;
  using namespace sci::cca::core;
  using namespace sci::cca::core::ports;

  /**
   * An implementation of the sci::cca::ports::FrameworkProperties
   * interface.
   * Provides access to framework properties that need to be exposed to
   * components such as:
   *    key                    value          meaning
   *    url                    string         this framework's url
   *    network file           string         network file to be loaded by a builder
   */
  
  template<class Interface>
  class FrameworkPropertiesBase : public Interface
  {
  public:
    virtual ~FrameworkPropertiesBase();
    
    /** ? */
    PortInfo::pointer getService(const std::string& ); 

    virtual Properties::pointer getProperties();
    virtual void setProperties(const Properties::pointer& properties);
    virtual void addProperties(const Properties::pointer& properties);
    virtual void removeProperties(const Properties::pointer& properties);

  protected:
    FrameworkPropertiesBase(const CoreFramework::pointer &framework);

    /** Gets user logged in on the controlling terminal of the process
        or a null pointer (see man getlogin(3)) */
    void getLogin();

    /**
     * Get SIDL file paths from (in order of processing):
     * the user's environment, CONFIG_FILE, component model defaults.
     *
     * Framework properties:
     * "sidl_xml_path": a ';' seperated list of directories where XML based
     *                  descriptions of components can be found.
     */
    void getSidlPaths();

    void parseEnvVariable(std::string& input, const char token,
                          SSIDL::array1<std::string>& stringArray);

    /** Persistent framework properties from file. */
    bool readPropertiesFromFile();

    /** Persistent framework properties from file. */
    bool writePropertiesToFile();

    Properties::pointer properties;
};

}

#endif
