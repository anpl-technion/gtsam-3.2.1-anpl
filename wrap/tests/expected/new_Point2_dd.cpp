// automatically generated by wrap on 2010-Feb-23
#include <wrap/matlab.h>
#include <Point2.h>
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("new_Point2_dd",nargout,nargin,2);
  double x = unwrap< double >(in[0]);
  double y = unwrap< double >(in[1]);
  Point2* self = new Point2(x,y);
  out[0] = wrap_constructed(self,"Point2");
}