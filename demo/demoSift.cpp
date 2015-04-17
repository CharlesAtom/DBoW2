/**
 * File: Demo.cpp
 * Date: November 2011
 * Author: Dorian Galvez-Lopez
 * Description: demo application of DBoW2
 * License: see the LICENSE.txt file
 */

#include <iostream>
#include <vector>
#include <dirent.h>

// DBoW2
#include "DBoW2.h" // defines Surf64Vocabulary and Surf64Database
// defines SiftVocabulary and SiftDatabase

#include <DUtils/DUtils.h>
#include <DUtilsCV/DUtilsCV.h> // defines macros CVXX
#include <DVision/DVision.h>

// OpenCV
#include <opencv/cv.h>
#include <opencv/highgui.h>
#if CV24
#include <opencv2/nonfree/features2d.hpp>
#endif


using namespace DBoW2;
using namespace DUtils;
using namespace std;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void loadFeatures(vector<vector<vector<float> > > &features,
  string sDatasetImagesDirectory, vector<string> imagesNames);
void storeImages(const char* imagesDirectory, vector<string>& imagesNames);
void changeStructure(const vector<float> &plain, vector<vector<float> > &out,
  int L);
void testVocCreation(const vector<vector<vector<float> > > &features,
  string& sOutDirectory);
void testDatabase(const vector<vector<vector<float> > > &datasetFeatures,
  const vector<vector<vector<float> > > &queryFeatures,
  vector<string>& datasetImagesNames, vector<string>& queryImagesNames,
  string& sOutDirectory);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// number of training images
int NIMAGES_DATASET;
int NIMAGES_QUERY;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void wait()
{
  cout << endl << "Press enter to continue" << endl;
  getchar();
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv)
{
  try {
    if (argc != 4) throw std::string("Invalid command line.");
  } catch (const std::string& s) {
      std::cerr << "Correct usage is: " << '\n'
      << "./demoSift <Dataset images directory> "
      << "<Query images directory> "
      << "<Output directory>" << '\n'
      << "Example: " << '\n'
      << "./demoSift ~/images/datasetImages ~/images/queryImages ~/images/output"
      << std::endl;

      std::cerr << s << std::endl;
      return EXIT_FAILURE;
  }

  const char* sDatasetImagesDirectory = argv[1];
  const char* sQueryImagesDirectory   = argv[2];
  string sOutDirectory = argv[3];

  vector<string> datasetImagesNames;
  vector<string> queryImagesNames;

  storeImages(sDatasetImagesDirectory, datasetImagesNames);
  storeImages(sQueryImagesDirectory  , queryImagesNames);

  NIMAGES_DATASET = datasetImagesNames.size();
  NIMAGES_QUERY   = queryImagesNames.size();

  cout << "Dataset images : " << endl;
  for (unsigned int i = 0; i < datasetImagesNames.size(); i++)
  {
      cout << "image " << i << " = " << datasetImagesNames[i] << endl;
  }

  cout << endl;

  cout << "Query images : " << endl;
  for (unsigned int i = 0; i < queryImagesNames.size(); i++)
  {
      cout << "image " << i << " = " << queryImagesNames[i] << endl;
  }

  cout << endl;
  cout << "Extracting SIFT features..." << endl;

  vector<vector<vector<float> > > datasetFeatures;
  vector<vector<vector<float> > > queryFeatures;
  loadFeatures(datasetFeatures, sDatasetImagesDirectory, datasetImagesNames);
  loadFeatures(queryFeatures  , sQueryImagesDirectory  , queryImagesNames  );

  testVocCreation(datasetFeatures, sOutDirectory);

  wait();

  testDatabase(datasetFeatures, queryFeatures, datasetImagesNames,
    queryImagesNames, sOutDirectory);

  return 0;
}

// ----------------------------------------------------------------------------

void storeImages(const char* imagesDirectory, vector<string>& imagesNames)
{
  DIR * repertoire = opendir(imagesDirectory);

  if ( repertoire == NULL)
  {
    cout << "The images directory" << imagesDirectory << " cannot be found" << endl;
  }
  else
  {
    struct dirent * ent;
    while ( (ent = readdir(repertoire)) != NULL)
    {
      if ((strncmp(ent->d_name, ".", 1) != 0)
      && (strncmp(ent->d_name, "..", 2) != 0))
      {
        imagesNames.push_back(ent->d_name);
      }
    }
  closedir(repertoire);
  }
}

// ----------------------------------------------------------------------------

void loadFeatures(vector<vector<vector<float> > > &features,
  string sDatasetDirectory, vector<string> imagesNames)
{
  features.clear();
  features.reserve(imagesNames.size());

  cv::SIFT sift(0, 3, 0.04, 10, 1.6);

  for(int i = 0; i < imagesNames.size(); ++i)
  {
    stringstream ss;
    ss << sDatasetDirectory << "/" << imagesNames[i];

    cv::Mat image = cv::imread(ss.str(), 0);
    cv::Mat mask;
    vector<cv::KeyPoint> keypoints;
    vector<float> descriptors;
    cv::Mat matDescriptors;
    sift(image, mask, keypoints, matDescriptors);

    descriptors.assign((float*)matDescriptors.datastart,
                       (float*)matDescriptors.dataend);

    features.push_back(vector<vector<float> >());
    changeStructure(descriptors, features.back(), sift.descriptorSize());
  }
}

// ----------------------------------------------------------------------------

void changeStructure(const vector<float> &plain, vector<vector<float> > &out,
  int L)
{
  out.resize(plain.size() / L);

  unsigned int j = 0;
  for(unsigned int i = 0; i < plain.size(); i += L, ++j)
  {
    out[j].resize(L);
    std::copy(plain.begin() + i, plain.begin() + i + L, out[j].begin());
  }
}

// ----------------------------------------------------------------------------

void testVocCreation(const vector<vector<vector<float> > > &features,
  string& sOutDirectory)
{
  // branching factor and depth levels
  const int k = 9;
  const int L = 3;
  const WeightingType weight = TF_IDF;
  const ScoringType score = L1_NORM;

  SiftVocabulary voc(k, L, weight, score);

  cout << "Creating a " << k << "^" << L << " vocabulary..." << endl;
  voc.create(features);
  cout << "... done!" << endl;

  cout << "Vocabulary information: " << endl
  << voc << endl << endl;

  // lets do something with this vocabulary
  cout << "Matching images against themselves (0 low, 1 high): " << endl;
  BowVector v1, v2;
  for(int i = 0; i < NIMAGES_DATASET; i++)
  {
    voc.transform(features[i], v1);
    for(int j = 0; j < NIMAGES_DATASET; j++)
    {
      voc.transform(features[j], v2);

      double score = voc.score(v1, v2);
      cout << "Image " << i << " vs Image " << j << ": " << score << endl;
    }
  }

  // save the vocabulary to disk
  cout << endl << "Saving vocabulary..." << endl;
  voc.save(sOutDirectory + "/voc.yml.gz");
  cout << "Done" << endl;
}

// ----------------------------------------------------------------------------

void testDatabase(const vector<vector<vector<float> > > &datasetFeatures,
  const vector<vector<vector<float> > > &queryFeatures,
  vector<string>& datasetImagesNames, vector<string>& queryImagesNames,
  string& sOutDirectory)
{
  cout << "Creating a database..." << endl;

  // load the vocabulary from disk
  SiftVocabulary voc(sOutDirectory + "/voc.yml.gz");

  SiftDatabase db(voc, false, 0); // false = do not use direct index
  // (so ignore the last param)
  // The direct index is useful if we want to retrieve the features that
  // belong to some vocabulary node.
  // db creates a copy of the vocabulary, we may get rid of "voc" now

  // add images to the database
  for(int i = 0; i < NIMAGES_DATASET; i++)
  {
    db.add(datasetFeatures[i]);
  }

  cout << "... done!" << endl;

  cout << "Database information: " << endl << db << endl;

  // and query the database
  cout << "Querying the database: " << endl;

  int nbBestMatchesToKeep = 4;

  QueryResults ret;
  for(int i = 0; i < NIMAGES_QUERY; i++)
  {
    db.query(queryFeatures[i], ret, nbBestMatchesToKeep);

    // ret[0] is always the same image in this case, because we added it to the
    // database. ret[1] is the second best match.

    for (int j = 0; j < nbBestMatchesToKeep; j++)
    {
      cout << "Searching for Image " << i << ": " << queryImagesNames[i] << "... Found : " << datasetImagesNames[ret[j].Id] << " with score " << ret[j].Score << endl;
    }
    cout << endl;
  }

  cout << endl;

  // we can save the database. The created file includes the vocabulary
  // and the entries added
  cout << "Saving database..." << endl;
  db.save(sOutDirectory + "/db.yml.gz");
  cout << "... done!" << endl;

  // once saved, we can load it again
  cout << "Retrieving database once again..." << endl;
  SiftDatabase db2(sOutDirectory + "/db.yml.gz");
  cout << "... done! This is: " << endl << db2 << endl;
}

// ----------------------------------------------------------------------------


