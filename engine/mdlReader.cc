/*
  @copyright Steve Keen 2020
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mdlReader.h"
#include "operation.h"
#include "userFunction.h"
#include "selection.h" //TODO why is this needed?
#include "minsky.h"
#include "minsky_epilogue.h"

#include <boost/locale.hpp>

using boost::locale::conv::utf_to_utf;
using namespace std;
namespace minsky
{
  namespace
  {
    string readToken(istream& mdlFile, char delim)
    {
      string r;
      string c;
      while (mdlFile>>GetUtf8Char(c))
        if (c[0]==delim)
          break;
        else if (c[0]>=' ') // strip out control characters
          r+=c;
      
      return trimWS(r);
    }

    bool identifierChar(int c)
    {return isalnum(c) || c=='\'' || c=='$';}
    
    // collapse multiple whitespace characters, and if in the middle
    // of an identifier (alphanum <space> alphanum, replace with
    // underscore, and tolower the characters (case insensitive)
    string collapseWS(const string& x)
    {
      // work in UTF-32 to simplify string analysis
      auto xx=utf_to_utf<uint32_t>(x);
      basic_string<uint32_t> result;
      uint32_t lastNonWS=0;
      bool quoted=false, lastWS=false;;
      for (auto& i: xx)
        {
          if (i=='"' &&lastNonWS!='\\')
            quoted=!quoted;
          
          if (!isspace(i) && i!='_')
            {
              // camelcase if collapsing whitespace in an identifier, ' ' otherwise
              if (lastWS)
                if (quoted || (identifierChar(i) && identifierChar(lastNonWS)))
                  if (isascii(i))
                    result+=toupper(i);
                  else
                    result+=i;
                else
                  result+=utf_to_utf<uint32_t>(" ")+i;
              else
                if (isascii(i))
                  result+=tolower(i);
                else
                  result+=i;
              lastNonWS=i;
              lastWS=false;
            }
          else
            lastWS=true;
        }
      return utf_to_utf<char>(result);
    }

    struct FunctionDef
    {
      std::string expression;
      std::function<void(const std::string&,exprtk::symbol_table<double>&)> addToTable;
    };

#define FUNCTION(def) [](const std::string& name, exprtk::symbol_table<double>& table){table.add_function(name,def);}       

    double pulse(double x, double y)
    {return (minsky().t>=x)*(minsky().t<x+y);}
    double pulseTrain(double s, double b,double r,double e)
    {
      double t=minsky().t; double tm=fmod(t,r); double sm=fmod(s,r); double bm=fmod(s+b,r);
      return (t<e)*(t>=s)*(tm>=sm)*(tm<bm);
    }
    
    map<string, FunctionDef> venSimFunctions={
      {"arccos",{"acos(x)",FUNCTION([](double x){return acos(x);})}},
      {"arcsin",{"asin(x)",FUNCTION([](double x){return asin(x);})}},
      {"arctan",{"atan(x)",FUNCTION([](double x){return atan(x);})}},
      {"gammaLn",{"gammaLn(x)",FUNCTION([](double x){return lgamma(x);})}},
      {"integer",{"floor(x)",FUNCTION([](double x){return floor(x);})}},
      {"ln",{"log(x)",FUNCTION([](double x){return log(x);})}},
      {"log",{"log(x)/log(y)",FUNCTION([](double x, double y){return log(x)/log(y);})}},
      {"modulo",{"modulo(x,y)",FUNCTION([](double x, double y){return fmod(x,y);})}},
      {"power",{"x^y",FUNCTION([](double x, double y){return pow(x,y);})}},
//      {"pulse",{"(time>=x)*(time<x+y)",FUNCTION([](double x, double y){return (minsky().t>=x)*(minsky().t<x+y);})}},
//      {"pulseTrain",{"pulseTrain(x,y,x,y)",FUNCTION([](double s, double b,double r,double e){
//        double t=minsky().t; double tm=fmod(t,r); double sm=fmod(s,r); double bm=fmod(s+b,r);
//        return (t<e)*(t>=s)*(tm>=sm)*(tm<bm);
//      })}},
      // why do the above lambdas fail?
      {"pulse",{"(time>=x)*(time<x+y)",FUNCTION(pulse)}},
      {"pulseTrain",{"pulseTrain(x,y,x,y)",FUNCTION(pulseTrain)}},
      {"quantum",{"floor(x/y)",FUNCTION([](double x, double y){return floor(x/y);})}},
      {"ramp",{"ramp(x,y,y)",FUNCTION([](double s, double t1, double t2){
        double t=minsky().t;
        if (t1>t2) swap(t1,t2);
        if (t<t1) return 0.0;
        else if (t<t2) return (t-t1)*s;
        else return (t2-t1)*s;
      })}},
      {"step",{"y*(time>=x)",FUNCTION([](double x,double y){return y*(minsky().t>x);})}},
      {"xidz",{"xidz(x,y,y)",FUNCTION([](double a,double b,double x){return (b==0)? x: a/b;})}},
      {"zidz",{"zidz(x,y)",FUNCTION([](double a,double b){return (b==0)? 0: a/b;})}}
    };

    set<string> functionsAdded; // track user functions added to group
    
    void addDefinitionToPort(Group& group, const shared_ptr<Port>& port, const string& name, const string& definition)
    {
      static regex identifier(R"([A-Za-z]\w*)");
      smatch match;
      if (regex_match(definition,match,identifier))
        {
          VariablePtr rhs(VariableBase::flow, definition);
          group.addItem(rhs);
          if (port)
            group.addWire(rhs->ports[0], port);
          return;
        }
      
      auto function=new UserFunction;
      group.addItem(function);
      function->description(name+"()");
      function->expression=definition;
      for (auto i: function->externalSymbolNames())
        {
          auto f=venSimFunctions.find(i);
          if (f!=venSimFunctions.end())
            {
              if (!functionsAdded.insert(f->first).second)
                addDefinitionToPort(group,nullptr,f->first,f->second.expression);
              f->second.addToTable(f->first, UserFunction::globalSymbols());
            }
       }
      if (port)
        group.addWire(function->ports[0], port);
    }
    
  }

  void readMdl(Group& group, RungeKutta& simParms, istream& mdlFile)
  {
    set<string> integrationVariables;
    regex integ(R"(\s*integ\s*\(([^,]*),([^,]*)\))");
    regex number(R"(\d*\.?\d+[Ee]?\d*)");
    regex unitFieldPattern(R"(([^\[\]]*)(\[.*\])?)");
    regex sliderSpecPattern(R"(\[([^,]*),?([^,]*),?([^,]*)\])");
    smatch match;
    UserFunction::globalSymbols().clear();
    functionsAdded.clear();

    UserFunction::globalSymbols().add_variable("time",minsky().t);
    UserFunction::globalSymbols().add_variable("timeStep",minsky().stepMax);
    UserFunction::globalSymbols().add_variable("initialTime",minsky().t0);
    UserFunction::globalSymbols().add_variable("finalTime",minsky().tmax);
    UserFunction::globalUnitSymbols().add_variable("time",timeUnit);
    
    string c;
    string currentMDLGroup;
    while (mdlFile>>GetUtf8Char(c))
      {
        if (isspace(c[0])) continue;
        switch (c[0])
          {
          case '{':
              // check this is a UTF-8 file
            if (readToken(mdlFile,'}')!="UTF-8")
              throw runtime_error("only UTF-8 file encoding is supported");
            continue;
        
          case '*':
            // group leadin
            for (; c[0]=='*'; mdlFile>>GetUtf8Char(c));
            currentMDLGroup=readToken(mdlFile,'*');
              
            while (mdlFile>>GetUtf8Char(c))
              if (c[0]=='|')
                {
                  c.clear();
                  break; // end of group leadin
                }
            break;
          }
        
        // macros?
        // read variable name
        string name=collapseWS(c+readToken(mdlFile,'='));
        if (name.substr(0,9)==R"(\\\---///)")
          break; // we don't parse the sketch information - not used in Minsky
        string definition=collapseWS(readToken(mdlFile,'~'));
        auto unitField=readToken(mdlFile,'~');
        regex_match(unitField,match,unitFieldPattern);
        string units=trimWS(match[1]);
        string sliderSpec;
        if (match.size()>1)
          sliderSpec=match[2];
        
        string comments=readToken(mdlFile,'|');


        if (currentMDLGroup==".Control")
          {
            if (name=="timeStep")
              {
                simParms.timeUnit=units;
                if (regex_match(definition,match,number))
                  simParms.stepMax=stod(definition);
              }
            if (!regex_match(definition,match,number)) continue;
            if (name=="initialTime") simParms.t0=stod(definition);
            if (name=="finalTime") simParms.tmax=stod(definition);
            continue;
          }
        if (regex_match(definition,match,integ))
          {
            auto intOp=new IntOp;
            group.addItem(intOp);
            intOp->description(name);
            intOp->detailedText=comments;
            auto& v=intOp->intVar;
            integrationVariables.insert(name);
            v->init(match[2].str());
            try  // absorb any errors in units - we have a chance to fix these later
              {
                v->setUnits(units);
              }
            catch (...) {}
            v->detailedText=comments;

            auto integrand=match[1].str();
            addDefinitionToPort(group, intOp->ports[1], "Integrand of "+name, integrand);
          }
        else if (regex_match(definition,match,number))
          {
            VariablePtr v(VariableBase::parameter, name);
            group.addItem(v);
            v->init(definition);
            try  // absorb any errors in units - we have a chance to fix these later
              {
                v->setUnits(units);
              }
            catch (...) {}
            v->detailedText=comments;
            if (regex_match(sliderSpec,match,sliderSpecPattern))
              {
                vector<string> spec;
                for (size_t i=1; i<=match.size(); ++i) spec.push_back(match[i]);
                if (spec.size()>0 && regex_match(spec[0],match,number))
                  v->sliderMin=stod(spec[0]);
                else
                  v->sliderMin=0.1*stod(definition);
                if (spec.size()>1 && regex_match(spec[1],match,number))
                  v->sliderMax=stod(spec[1]);
                else
                  v->sliderMax=10*stod(definition);
                if (spec.size()>2 && regex_match(spec[2],match,number))
                  v->sliderStep=stod(spec[2]);
                v->adjustSliderBounds();
              }
            else
              v->initSliderBounds();
          }
        else
          {
            VariablePtr v(VariableBase::flow, name);
            group.addItem(v);
            addDefinitionToPort(group, v->ports[1], "Def: "+name, definition);
            try  // absorb any errors in units - we have a chance to fix these later
              {
                v->setUnits(units);
              }
            catch (...) {}
            v->detailedText=comments;
          }
      }

  }
}