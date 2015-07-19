##Summarizing the PCA approach
Listed below are the 6 general steps for performing a principal component analysis, which we will investigate in the following sections.

1. Take the whole dataset consisting of ***d***-dimensional samples ignoring the class labels
2. Compute the ***d***-dimensional mean vector (i.e., the means for every dimension of the whole dataset)
3. Compute the scatter matrix (alternatively, the covariance matrix) of the whole data set
4. Compute eigenvectors (**e**<sub>1</sub>,**e**<sub>2</sub>,...,**e**<sub>*d*</sub>) and corresponding eigenvalues (**λ**<sub>1</sub>,**λ**<sub>2</sub>,...,**λ**<sub>*d*</sub>)
5. Sort the eigenvectors by decreasing eigenvalues and choose ***k*** eigenvectors with the largest eigenvalues to form a d×k dimensional matrix **W** (where every column represents an eigenvector)
6. Use this ***d×k*** eigenvector matrix to transform the samples onto the new subspace. This can be summarized by the mathematical equation: **y=W<sup>*T*</sup> × x** (where x is a ***d×1***-dimensional vector representing one sample, and ***y*** is the transformed ***k×1***-dimensional sample in the new subspace.)

###Reference
* [Implementing a Principal Component Analysis](http://sebastianraschka.com/Articles/2014_pca_step_by_step.html)
* [The mathematical principles of PCA](http://dataunion.org/13702.html)