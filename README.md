# `BayesLang`

This contains the source code (`C++`) for `BayesLang`, a Bayesian phylogenetic inference software designed for typological data and used for a 2011 paper (and a few others based on its results):

> Dediu, D. (2011). A Bayesian phylogenetic approach to estimating the stability of linguistic features and the genetic biasing of tone. Proc R Soc B, 278, 474–479. https://doi.org/10.1098/rspb.2010.1595

> Dediu, D., & Levinson, S. C. (2012). Abstract Profiles of Structural Stability Point to Universal Tendencies, Family-Specific Factors, and Ancient Connections between Languages. PLoS ONE, 7(9), e45198. https://doi.org/10.1371/journal.pone.0045198

and

> Dediu, D., & Cysouw, M. (2013). Some Structural Aspects of Language Are More Stable than Others: A Comparison of Seven Methods. PLoS ONE, 8(1), e55009. https://doi.org/10.1371/journal.pone.0055009

This source code has been available since 2011 at a different location, now defunct...

-------

## The original ReadMe.txt file

BayesLang is a Bayesian phylogeny inference program designed for typological linguistic data. It was designed and written in order to study the stability of typological features (Dediu D., 2011, A Bayesian phylogenetic approach to estimating the stability of linguistic features and the genetic biasing of tone, Proc. Royal Soc. B.) but it can be used for other goals as well (e.g., the inference of ancestral states). 

It implements a standard Bayesian phylogeny inference algorithm through a multi-threaded Metropolis-Coupled Markov Chain Monte Carlo, with each chain running in a different thread (or CPU/core on multi-CPU/multi-core machines), with the number of hot chains specified by the user. More information can be found in the cited paper and especially in the accompanying Electronic Supplementary Material.

Its input data is organized as a standard Nexus file (processed using the NEXUS Class Library, http://sourceforge.net/projects/ncl), except for the presence of two custom blocks, "trees" and "topology". A single such file contains the information for all the language families to be processed. A toy example - Test1.nex - is given below (see also the real Nexus files described further on):

```
#nexus

[This file must contain the languages, features, data matrix (actual feature values for all languages) and tree/classification temples.]

begin taxa; [the languages]
	dimensions ntax=11;
	taxlabels Lang1 Lang2 Lang3 Lang4 Lang5 Lang6 Lang7 Lang8 Lang9 Lang10 Lang11; [Required!]
end;

begin characters; [the linguistic features and the data matrix, allowed symbol values 0..9]
  dimensions nchar=6;
  charlabels Feat1 [0,1] Feat2 [0,1] Feat3 [0,1,2,3] Feat4 [0,1,2,3] Feat5 [0,1,2,3] Feat6 [0,1,2,3]; [Required!]
  format datatype=standard missing=. symbols=". 0 1 2 3 4 5 6 7 8 9";
  matrix
	Lang1    0 1 3 0 0 0
	Lang2    1 0 0 2 2 2
	Lang3    0 0 2 1 1 1
	Lang4    0 1 2 . . .
	Lang5    1 0 0 3 3 3
	Lang6    1 1 1 3 3 3
	Lang7    0 0 3 1 1 1
	Lang8    0 1 0 0 0 0
	Lang9    1 0 . 2 2 2
	Lang10   1 1 3 3 3 3
	Lang11   1 1 1 0 0 0
  ;
end;

begin trees [the tree/classification temples];
	tree Family1 = ((Lang1,Lang2,Lang3,Lang4)Proto1234,((Lang5,Lang6)Proto56,(Lang7,Lang8)Proto78)Proto5678)Proto12345678;
	tree Family2 = ((Lang9,Lang10)Proto910,Lang11)Proto91011;
end;

begin typology; [linguistic typology specific info]
  chartypes Unordered:Feat1 Feat2, Ordered: Feat3, Circular: Feat4, Ranked: Feat5, Custom: Feat6;
  custommatrix Feat6 4
    - 1 0 0  [- 1 1 0]
    1 - 1 0  [1 - 0 1]
    0 1 - 1  [1 0 - 1]
    0 0 1 -  [0 1 1 -]
  ;
  valuelabels Feat1: 0 1, Feat2: 0 1, Feat3: 0 1 2 3, Feat4: 0 1 2 3, Feat5: 0 1 2 3, Feat6: 0 1 2 3;
end;
```

The "trees" block represents the language families in a slightly customized Newick tree format, giving also the proto-language names. The "topology" block contains the types of the linguistic features, the custom instantaneous transition matrices for the custom features and the names of the feature values. 

The program uses the the `NCL 2.1.04` library by P.O. Lewis (http://sourceforge.net/projects/ncl), and on `Windows` the `pthreads-win32` library by Ross Johnson. It also contains some code based on `MrBayes 3.1.2` by J.P. Huelsenbeck & F. Ronquist for the generation of the initial branch lengths, the rate multiplier and especially the computation of matrix exponentiation (Golub, G. H., and C. F. Van Loan. 1996. Matrix Computations, Third Edition. The Johns Hopkins University Press, Baltimore, Maryland.), but the core algorithms are based on the primary literature as described in the paper.

`BayesLang` is released under GPL v2 (see file `gpl.txt`) and I hope that this code will help in other implementations of related models.

The program accepts the following parameters:

```
  BayesLang -ngens=n1 -nchains=n2 nf -trees
     nf: Nexus file containing the languages, features and templates (see above)
     n1: the number of generations to run (default 1000000)
     n2: the number of chains (at least 1; default 4)
     -trees: save the trees in the results file
```

and produces a number of output files. Running

```
  BayesLang -ngen=10000 -nchains=4 Test1.nex -trees
```

produces the fllowing output:

```
BayesLang (c) Dan Dediu, 2008, ddediu@gmail.com
Max Planck Institute for Psycholinguistics, Nijmegen

This is released under GPL (see GPL.txt for details).
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details (www.gnu.org and gpl.txt).

Contains code based on "MrBayes 3.1.2" (c) J.P. Huelsenbeck & F. Ronquist
Uses the "NCL 2.1.04" library (c) P.O. Lewis
The Windows version uses the "pthreads-win32" library by Ross Johnson released under LGPL (for details, see lgpl.txt)

Usage:
  BayesLang -ngens=n1 -nchains=n2 nf -trees
where:
  nf: Nexus file containing the languages, features and templates (see manual)
  n1: the number of generations to run (default 1000000)
  n2: the number of chains (at least 1; default 4)
  -trees: save the trees in the results file
--------------------------------------------------------------------------------


SMP Implmentation (uses pthreads)...

Runing 10000 generations...
Runing 4 chains...
Reading & parsing Nexus input file "Test1.nex"...

Reading "TAXA" block...
storing read block: TAXA
Reading "CHARACTERS" block...
storing read block: CHARACTERS
Reading "TREES" block...
storing read block: TREES
Reading "TYPOLOGY" block...
storing read block: TYPOLOGY

The current model:
There are 11 languages: Lang1,Lang2,Lang3,Lang4,Lang5,Lang6,Lang7,Lang8,Lang9,Lang10,Lang11
There are 6 features: Feat1[U,0-1],Feat2[U,0-1],Feat3[O,0-3],Feat4[C,0-3],Feat5[R,0-3],Feat6[K,0-3]
 L\F  0   1   2   3   4   5 
  0   0   1   3   0   0   0 
  1   1   0   0   2   2   2 
  2   0   0   2   1   1   1 
  3   0   1   2   .   .   . 
  4   1   0   0   3   3   3 
  5   1   1   1   3   3   3 
  6   0   0   3   1   1   1 
  7   0   1   0   0   0   0 
  8   1   0   .   2   2   2 
  9   1   1   3   3   3   3 
 10   1   1   1   0   0   0 

There are 2 tree templates:
  Family1:


Proto12345678  |.?|.?|.?|.?|.?|.?| t=0.000
|  
+--Proto1234  |.?|.?|.?|.?|.?|.?| t=0.041
|  |  
|  +--Lang1  |0!|1!|3!|0!|0!|0!| t=0.047
|  |  
|  +--Lang2  |1!|0!|0!|2!|2!|2!| t=0.069
|  |  
|  +--Lang3  |0!|0!|2!|1!|1!|1!| t=0.070
|  |  
|  \--Lang4  |0!|1!|2!|..|..|..| t=0.099
|  
\--Proto5678  |.?|.?|.?|.?|.?|.?| t=0.048
   |  
   +--Proto56  |.?|.?|.?|.?|.?|.?| t=0.074
   |  |  
   |  +--Lang5  |1!|0!|0!|3!|3!|3!| t=0.128
   |  |  
   |  \--Lang6  |1!|1!|1!|3!|3!|3!| t=0.015
   |  
   \--Proto78  |.?|.?|.?|.?|.?|.?| t=0.145
      |  
      +--Lang7  |0!|0!|3!|1!|1!|1!| t=0.059
      |  
      \--Lang8  |0!|1!|0!|0!|0!|0!| t=0.001

  Family2:


Proto91011  |.?|.?|.?|.?|.?|.?| t=0.000
|  
+--Proto910  |.?|.?|.?|.?|.?|.?| t=0.167
|  |  
|  +--Lang9  |1!|0!|..|2!|2!|2!| t=0.044
|  |  
|  \--Lang10  |1!|1!|3!|3!|3!|3!| t=0.001
|  
\--Lang11  |1!|1!|1!|0!|0!|0!| t=0.001




Processing tree Family1 (1 out of 2)...Current MCMC progress (0 to 9): 0123456789
 so far took: 1 seconds
Processing tree Family2 (2 out of 2)...Current MCMC progress (0 to 9): 0123456789
 so far took: 2 seconds

Time taken: 2
```


This output details the input model, the chain progress (this can take a long while if the model is complex and the number of generations high) as well as the total run time. The output files produced are:

  - auxiliary files:

     - `MCMCFeaturesInfo.csv`: this file lists the individual features, their type and the range of values;
     - `MCMCModelStructure-*.csv`: for each defined family, a file containing the ancestor-descendant matrix;
     - `MCMCTreesInfo.csv`: if parameter -trees was specified, this file simply lists all the language families processed;
     
  - actual result files:
  
     - `MCMCResults-*1.csv`: for each family, this file contains the posterior samples detailing the number of consecutive times this tree has been sampled (to save disk space), the first generation this tree appeared, the actual tree in the customized Newick format described above, the log likelihood of the tree, the number of changes per feature and for all feature for the whole tree, the ancestral states and the number of changes between specific nodes in the tree, and, finally, the value of alpha for each feature.

These output files can be further processed to extract the relevant information (in this paper's case, the number of changes per feature for the whole tree).

The code is cross-platform `C++` and it complies under various flavours of Linux (it has been developped, tested and run mainly on `Suse Linux 10.1` x64 using `CGG`, `GDB` and `Code::Blocks` -- `BayesLang.cbp` is the `Code::Blocks` project file) and also on `Windows` using `MingW`. The code has been optimized and the performance is acceptable. However, this code should be regarded as a platform for developing and testing new ideas rather than a finished product ready for daily use!


Also included in this package are the real data used in the paper: in folder `Dediu2011-data` there are the 4 files `ethnologue-binary.nex`, `wals-binary.nex`, `ethnologue-polymorphic.nex` and `wals-poymorphic.nex`, each containing the Nexus files for one of the 4 scenarios described in the paper. Please note that "taxa" section of these files contains languages which are not part of any used language family (due to the process of extracting these data from the WALS and Ethnologue databases and exporting it in this Nexus format), which explains the discordance between the number of taxa ("dimensions ntax") and the number reported in the paper (and acutally used in the inference process). 



