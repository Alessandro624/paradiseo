#include <eo>
#include <do>

#include "Rosenbrock.h"

typedef eoReal< eoMinimizingFitness > EOT;

int main(void)
{
    doBounderNo< EOT > bounder;

    return 0;
}
