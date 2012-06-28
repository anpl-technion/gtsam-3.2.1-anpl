% automatically generated by wrap
classdef Point3 < handle
  properties
    self = 0
  end
  methods
    function obj = Point3(varargin)
      if (nargin == 3 && isa(varargin{1},'double') && isa(varargin{2},'double') && isa(varargin{3},'double')), obj.self = new_Point3_(0,0,varargin{1},varargin{2},varargin{3}); end
      if nargin ==14, new_Point3_(varargin{1},0); end
      if nargin ~= 13 && nargin ~= 14 && obj.self == 0, error('Point3 constructor failed'); end
    end
    function delete(obj)
      if obj.self ~= 0
        new_Point3_(obj.self);
        obj.self = 0;
      end
    end
    function display(obj), obj.print(''); end
    function disp(obj), obj.display; end
  end
end
