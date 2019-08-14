#include "InfoHolder.h"

//The features info file:
const char *InfoHolder::st_FeaturesFileName = "MCMCFeaturesInfo.csv";

InfoHolder::InfoHolder()
{
    //ctor
}

InfoHolder::~InfoHolder()
{
    //dtor
}


//Init the infor holder:
void InfoHolder::Init( NxsTaxaBlock *iNxsTaxaBlock, NxsCharactersBlock *iNxsCharactersBlock, NxsTypologyBlock *iNxsTypologyBlock, NxsTreesBlock *iNxsTreesBlock )
{
    SetLanguages( iNxsTaxaBlock );
    SetFeatures( iNxsTypologyBlock );
    SetValuesMatrix( iNxsCharactersBlock );
    SetTreeTemplates( iNxsTreesBlock );
}

//Set the languages:
void InfoHolder::SetLanguages( NxsTaxaBlock *iNxsTaxaBlock )
{
    unsigned nLanguages = iNxsTaxaBlock->GetNTax();

    if( nLanguages == 0 )
    {
        cerr << "At least one language is needed!" << endl;
        assert( false );
        exit( 1 );
    }

    m_LanguagesSet.Init( nLanguages );

    for( unsigned i = 0; i < nLanguages; ++i )
    {
        const char* languageName = iNxsTaxaBlock->GetTaxonLabel(i).c_str();
        if( !languageName )
        {
            cerr << "All languages must have a label!" << endl;
            assert( false );
            exit( 1 );
        }

        m_LanguagesSet.GetLanguage(i)->Init( languageName );
    }
}


//Set the features:
void InfoHolder::SetFeatures( NxsTypologyBlock *iNxsTypologyBlock )
{
    unsigned nFeatures = iNxsTypologyBlock->GetNFeatures();

    if( nFeatures == 0 )
    {
        cerr << "At least one feature is needed!" << endl;
        assert( false );
        exit( 1 );
    }

    m_FeaturesSet.Init( nFeatures );

    for( unsigned i = 0; i < nFeatures; ++i )
    {
        const char* featureName = iNxsTypologyBlock->GetFeatureLabel(i);
        if( !featureName )
        {
            cerr << "All features must have a label!" << endl;
            assert( false );
            exit( 1 );
        }

        m_FeaturesSet.GetFeature(i)->Init( featureName, (iNxsTypologyBlock->GetFeatureType(i) == NxsTypologyBlock::TYPOLOGICAL_CHAR_UNORDERED) ? Feature::FEATURE_UNORDERED :
                                                        (iNxsTypologyBlock->GetFeatureType(i) == NxsTypologyBlock::TYPOLOGICAL_CHAR_CIRCULAR) ? Feature::FEATURE_CIRCULAR :
                                                        (iNxsTypologyBlock->GetFeatureType(i) == NxsTypologyBlock::TYPOLOGICAL_CHAR_RANKED) ? Feature::FEATURE_RANKED :
                                                        (iNxsTypologyBlock->GetFeatureType(i) == NxsTypologyBlock::TYPOLOGICAL_CHAR_CUSTOM) ? Feature::FEATURE_CUSTOM :
                                                        Feature::FEATURE_ORDERED,
                                           '0', '0' + iNxsTypologyBlock->GetFeatureValueLabels(i).size()-1,
                                           iNxsTypologyBlock->GetFeatureValueLabels(i) );

        if( iNxsTypologyBlock->GetFeatureType(i) == NxsTypologyBlock::TYPOLOGICAL_CHAR_CUSTOM )
        {
            m_FeaturesSet.GetFeature(i)->SetCustomTransitionsMatrix( iNxsTypologyBlock->GetFeatureCustomTransitionsMatrix(i) );
        }
    }
}

//Set the values matrix:
void InfoHolder::SetValuesMatrix( NxsCharactersBlock *iNxsCharactersBlock )
{
    //The languages and feature set must have been initialized beforehand:
    assert( m_LanguagesSet.GetNLanguages() > 0 && m_FeaturesSet.GetNFeatures() );

    //Prepare the values matrix:
    m_ValuesMatrix.Init( m_LanguagesSet.GetNLanguages(), m_FeaturesSet.GetNFeatures(), &m_LanguagesSet, &m_FeaturesSet );

    //Set the values:
    unsigned i,j;
    const char *states = iNxsCharactersBlock->GetSymbols();
    if( !states )
    {
        cerr << "Values matrix symbols must be defined!" << endl;
        assert( false );
        exit( 1 );
    }

    char missingState = iNxsCharactersBlock->GetMissingSymbol();
    if( missingState == '\0' )
    {
        cerr << "The missing value must be defined!" << endl;
        assert( false );
        exit( 1 );
    }

    for( i = 0; i < m_LanguagesSet.GetNLanguages(); ++i )
    {
        for( j = 0; j < m_FeaturesSet.GetNFeatures(); ++j )
        {
            int value = FeatureValuesMatrix::c_MissingData;
            char state = iNxsCharactersBlock->GetState(i,j);

            if( state != missingState )
            {
                //Check if the value is legal:
                if( state < '0' || state > '9' )
                {
                    cerr << "Feature values must be between \'0\' and \'9\': illegal value \'" << (char)state <<"\' found!" << endl;
                    assert( false );
                    exit( 1 );
                }

                //Store it as such!
                value = (int)state;
            }

            m_ValuesMatrix.SetValue( i, j, value );
        }
    }

    /*//Adjust the maximum value for each feature given those present in the matrix:
    for( j = 0; j < m_FeaturesSet.GetNFeatures(); ++j )
    {
        int maxValue = (-1);
        int minValue = 1024;
        for( i = 0; i < m_LanguagesSet.GetNLanguages(); ++i )
        {
            maxValue = max( maxValue, m_ValuesMatrix.GetValue( i, j ) );
            minValue = min( minValue, m_ValuesMatrix.GetValue( i, j ) == FeatureValuesMatrix::c_MissingData ? minValue : m_ValuesMatrix.GetValue( i, j ) );
        }
        m_FeaturesSet.GetFeature(j)->SetMinMaxVals( minValue, maxValue );
    }*/

    //Save the features info to file for later processing:
    FILE *file = fopen( st_FeaturesFileName, "wt" );
    if( !file )
    {
        cerr << "Cannot create features file!" << endl;
        assert( false );
        exit( 1 );
    }

    //Write the header:
    fprintf( file, "Name\tType\tMinValue\tMaxValue\tNVals\n" );

    for( j = 0; j < m_FeaturesSet.GetNFeatures(); ++j )
    {
        fprintf( file, "%s\t%s\t%d\t%d\t%d\n", m_FeaturesSet.GetFeature(j)->GetName(),
                 (m_FeaturesSet.GetFeature(j)->GetType() == Feature::FEATURE_UNORDERED) ? "UNORDERED" : (m_FeaturesSet.GetFeature(j)->GetType() == Feature::FEATURE_ORDERED) ? "ORDERED" :
                 (m_FeaturesSet.GetFeature(j)->GetType() == Feature::FEATURE_CIRCULAR) ? "CIRCULAR" : (m_FeaturesSet.GetFeature(j)->GetType() == Feature::FEATURE_CUSTOM) ? "CUSTOM" : "RANKED",
                 m_FeaturesSet.GetFeature(j)->GetMinVal(), m_FeaturesSet.GetFeature(j)->GetMaxVal(), m_FeaturesSet.GetFeature(j)->GetNVals() );
    }

    fclose( file );
}
//Set the tree templates:
void InfoHolder::SetTreeTemplates( NxsTreesBlock *iNxsTreesBlock )
{
    unsigned nTrees = iNxsTreesBlock->GetNumTrees();
    m_TreeTemplatesSet.Init( nTrees );

    for( unsigned i = 0; i < nTrees; ++i )
    {
        //Get the tree the tree:
        const NxsFullTreeDescription &treeDescription = iNxsTreesBlock->GetFullTreeDescription(i);
        m_TreeTemplatesSet.GetTreeTemplate(i)->SetName( treeDescription.GetName().c_str() );

        //Parse it:
        NxsSimpleTree tree( treeDescription, -1, -1.0 );

        std::vector<const NxsSimpleNode *> nodes = tree.GetPreorderTraversal();

        /*cout << nodes.size() << " ";
        for( unsigned i = 0; i < nodes.size(); ++i )
        {
            NxsSimpleEdge edge = nodes[i]->GetEdgeToParent();
            cout << "[" << nodes[i]->GetTaxIndex() << "," << ((nodes[i]->GetName() && *nodes[i]->GetName()) ? nodes[i]->GetName() : m_LanguagesSet.GetLanguage(nodes[i]->GetTaxIndex())->GetName()) << ":" << edge.GetDblEdgeLen() << "] ";
        }*/

        //Test to see if the tree is empty:
        if( nodes.size() == 0 )
        {
            cerr << "Tree \"" << treeDescription.GetName() << "\" cannot be empty!" << endl;
            assert( false );
            exit( 1 );
        }

        //Get the root:
        const NxsSimpleNode *root = nodes[0];
        //and add it recursively to the tree:
        m_TreeTemplatesSet.GetTreeTemplate(i)->AddTree( root, m_LanguagesSet, m_ValuesMatrix, &m_FeaturesSet );

        //tree.WriteAsNewick( cout, true );
    }
}

//Print it to file:
void InfoHolder::Print( FILE *iFile )
{
    assert( iFile );

    fprintf( iFile, "\nThe current model:\n" );

    m_LanguagesSet.Print( iFile );
    m_FeaturesSet.Print( iFile );
    m_ValuesMatrix.Print( iFile );
    m_TreeTemplatesSet.Print( iFile, false, -1 );

    fprintf( iFile, "\n\n" );
    fflush( iFile );
}




