//-----------------------------------------------------------------------------
// mlp.h
//-----------------------------------------------------------------------------

#ifndef mlp_h
#define mlp_h

//-----------------------------------------------------------------------------

using namespace std;
#include <values.h>               // MAXFLOAT MINFLOAT
#include <math.h>                 // exp
#include <stdexcept>              // invalid_argument
#include <iostream>               // istream ostream
#include <algorithm>              // generate
#include <vector>                 // vector
#include <utils/eoRNG.h>          // eoRng
#include <utils/rnd_generators.h> // normal_geneurator
#include <vecop.h>                // *
#include <utility>
#include <iterator>

//-----------------------------------------------------------------------------

namespace mlp
{
  typedef double            real;
  typedef std::vector<real> vector;
}

namespace std {
  ostream& operator<<(ostream& os, const mlp::vector& v)
  {
    ostream_iterator<mlp::real> oi(os, " ");
    copy(v.begin(), v.end(), oi);
    return os;
  }
                                                                                
  istream& operator>>(istream& is, mlp::vector& v)
  {
    for (mlp::vector::iterator vi = v.begin() ; vi != v.end() ; vi++) {
        is >> *vi;
    }
    return is;
  }
}

namespace mlp
{
  //---------------------------------------------------------------------------
  // useful typedefs
  //---------------------------------------------------------------------------
  
  
  const real max_real = MAXFLOAT;
  const real min_real = MINFLOAT;


  //---------------------------------------------------------------------------
  // sigmoid
  //---------------------------------------------------------------------------
  
  real sigmoid(const real& x)
  {
    return 1.0 / (1.0 + exp(-x));
  }
  
  
  //---------------------------------------------------------------------------
  // neuron
  //---------------------------------------------------------------------------
  
  struct neuron
  {
    real   bias;
    vector weight;
    
    neuron(const unsigned& num_inputs = 0): weight(num_inputs) {}
    
    void reset()
    {
      normal_generator<real> rnd(1.0);
      bias = rnd();
      generate(weight.begin(), weight.end(), rnd);
    }
    
    real operator()(const vector& input) const
    {
      return sigmoid(bias + weight * input);
    }

    unsigned length() const { return weight.size() + 1; }
    
    void normalize()
    {
      real n = sqrt(bias * bias + weight * weight);
      bias /= n;
      weight /= n;
    }

    void desaturate()
    {
      bias = -5.0 + 10.0 / (1.0 + exp(bias / -5.0));
      
      for (vector::iterator w = weight.begin(); w != weight.end(); ++w)
	*w = -5.0 + 10.0 / (1.0 + exp(*w / -5.0));
    }
  };
}

namespace std {

  ostream& operator<<(ostream& os, const mlp::neuron& n)
  {
    return os << n.bias << " " << n.weight;
  }
                                                                                
  istream& operator>>(istream& is, mlp::neuron& n)
  {
    return is >> n.bias >> n.weight;
  }

}

namespace mlp {
  
  //---------------------------------------------------------------------------
  // layer
  //---------------------------------------------------------------------------
  
  class layer: public std::vector<neuron>
  {
  public:
    layer(const unsigned& num_inputs = 0, const unsigned& num_neurons = 0): 
      std::vector<neuron>(num_neurons, neuron(num_inputs)) {}

    void reset()
    {
      normal_generator<real> rnd(1.0);
      for(iterator n = begin(); n != end(); ++n)
	n->reset();
    }
    
    vector operator()(const vector& input) const
    {
      vector output(size());
      
      for(unsigned i = 0; i < output.size(); ++i)
	output[i] = (*this)[i](input);

      return output;
    }

    unsigned length() const { return front().length() * size(); }

    void normalize()
    {
      for(iterator n = begin(); n != end(); ++n)
	n->normalize();
    }

    void desaturate()
    {
      for(iterator n = begin(); n != end(); ++n)
	n->desaturate();
    }
  };

}

namespace std {

  ostream& operator<<(ostream& os, const mlp::layer& l)
  {
    ostream_iterator<mlp::neuron> oi(os, " ");
    copy(l.begin(), l.end(), oi);
    return os;
  }
                                                                                
  istream& operator>>(istream& is, mlp::layer& l)
  {
    for (mlp::layer::iterator li = l.begin() ; li != l.end() ; li++) {
        is >> *li;
    }
    return is;
  }

}

namespace mlp {
  

  //---------------------------------------------------------------------------
  // net
  //---------------------------------------------------------------------------

  class net: public std::vector<layer>
  {
  public:
    net(const unsigned& num_inputs = 0,
	const unsigned& num_outputs = 0, 
	const std::vector<unsigned>& hidden = std::vector<unsigned>())
    {
      init(num_inputs,num_outputs,hidden);
    }


    net(istream &is) {
	load(is);
    }

    void load(istream &is) {
	unsigned num_inputs;
	unsigned num_outputs;
	unsigned num_hidden_layers;

	is >> num_inputs >> num_outputs >> num_hidden_layers;

	std::vector<unsigned> layer_sizes;
	for (int i=0; i<num_hidden_layers;i++) {
	   unsigned layer_size;
	   is >> layer_size;
	   layer_sizes.push_back(layer_size);
	}
        unsigned check_outputs;
	is >> check_outputs;
	assert (check_outputs ==  num_outputs);
	init (num_inputs,num_outputs,layer_sizes);
        // skip forward to pass up opening '<' char
        char c=' ';
        while (c!='<') { is >> c;}
        for (iterator l =begin() ; l != end(); l++) {
	    is >> *l;
	}
    }

    void init( unsigned num_inputs, 
	       unsigned num_outputs, 
	       const std::vector<unsigned>& hidden ) {
      clear();
      switch(hidden.size())
	{
	case 0:
	  push_back(layer(num_inputs, num_outputs));
	  break;
	default:
	  push_back(layer(num_inputs, hidden.front()));
	  for (unsigned i = 0; i < hidden.size() - 1; ++i)
	    push_back(layer(hidden[i], hidden[i + 1]));
	  push_back(layer(hidden.back(), num_outputs));
	  break;
	}
    }

    void reset()
    {
      normal_generator<real> rnd(1.0);
      for(iterator l = begin(); l != end(); ++l)
	l->reset();
    }

    virtual vector operator()(const vector& input) const ;

    unsigned winner(const vector& input) const
    {
      vector tmp = (*this)(input);
      return (max_element(tmp.begin(), tmp.end()) - tmp.begin());
    }

    void save(ostream &os) const {
        // Save the number of inputs, number of outputs, and number of hidden layers
	os << num_inputs() << "\n" << num_outputs() << "\n" << num_hidden_layers() << "\n";
        for(const_iterator l = begin(); l != end(); ++l)
	   os << l->size() << " ";
        os << "\n";
	os << *this;
        os << "\n";
    }

    unsigned num_inputs()  const { return front().front().length() - 1; }
    unsigned num_outputs() const { return back().size(); }
    unsigned num_hidden_layers() const { 
	signed s = (signed) size() -1; 
	return (s<0) ? 0 : s ;
    }


    unsigned length() 
    {
      unsigned sum = 0;

      for(iterator l = begin(); l != end(); ++l)
	sum += l->length();
      
      return sum; 
    } 

    void normalize()
    {
      for(iterator l = begin(); l != end(); ++l)
	l->normalize();
    }

    void desaturate()
    {
      for(iterator l = begin(); l != end(); ++l)
	l->desaturate();
    }
  };

#ifndef NO_MLP_VIRTUALS
    vector net::operator()(const vector& input) const
    {
      vector tmp = input;
      
      for(const_iterator l = begin(); l != end(); ++l)
	tmp = (*l)(tmp);
      
      return tmp;
    }   
#endif


  //---------------------------------------------------------------------------
  // sample
  //---------------------------------------------------------------------------
  
  struct sample
  {
    vector input, output;
    
    sample(unsigned input_size = 0, unsigned output_size = 0):
      input(input_size), output(output_size) {}
  };
  
  istream& operator>>(istream& is, sample& s)
  {
    return is >> s.input >> s.output;
  }
  
  ostream& operator<<(ostream& os, const sample& s)
  {
    return os << s.input << " " << s.output;
  }

  
  //---------------------------------------------------------------------------
  // set
  //---------------------------------------------------------------------------

  class set: public std::vector<sample>
  {
  public:
    set(unsigned input_size  = 0, unsigned output_size = 0, 
	unsigned num_samples = 0): 
      std::vector<sample>(num_samples, sample(input_size, output_size)) {}

    set(istream& is) : std::vector<sample>(0, sample(0, 0)) {
	clear();
	load(is);
    }

    void load(istream &is) {
	unsigned input_size, output_size;
        is >> input_size >> output_size;
	sample samp(input_size, output_size);;
	while (is >> samp) { push_back(samp); }
    }

    void save(ostream &os) const {
	os << front().input.size() << " " << front().output.size() << endl;
	copy(begin(), end(), ostream_iterator<sample>(os,"\n"));
    }
  };

  ostream& operator<<(ostream& os, const set& s)
  {
    os << "<" << endl;
    for (unsigned i = 0; i < s.size(); ++i)
      os << s[i] << endl;
    return os << ">";
  }

  //---------------------------------------------------------------------------
  // euclidean_distance
  //---------------------------------------------------------------------------

  real euclidean_distance(const net& n1, const net& n2)
  {
    real sum = 0;

    for(net::const_reverse_iterator l1 = n1.rbegin(), l2 = n2.rbegin(); 
	l1 != n1.rend() && l2 != n2.rend(); ++l1, ++l2)
      for(layer::const_iterator n1 = l1->begin(), n2 = l2->begin(); 
	  n1 != l1->end() && n2 != l2->end(); ++n1, ++n2)
	{
	  real b = n1->bias - n2->bias;
	  vector w = n1->weight - n2->weight;
	  sum += b * b + w * w;
	}
    /*
      #include <fstream>
      std::ofstream file("dist.stat", ios::app);
      file << sqrt(sum) << endl;
    */
    return sqrt(sum);
  }

  //---------------------------------------------------------------------------

} // namespace mlp

//-----------------------------------------------------------------------------

#endif // mlp_h

// Local Variables: 
// mode:C++ 
// End:
