# Makefile for the BayesLang project

CC = g++

# Release:
CFLAGS = -Wall -O3  -march=nocona  -fexpensive-optimizations  

COMPILE = $(CC) $(CFLAGS) -c


all: BayesLang



BayesLang: main.o Feature.o FeatureRateMatricesSet.o FeatureRateMatrix.o FeatureValuesMatrix.o FeaturesSet.o InfoHolder.o LangNexusReader.o LangNexusToken.o Language.o LanguageTreeTemplate.o LanguageWithFeatures.o LanguagesSet.o MarkovChain.o MetropolisCoupledMCMC.o NxsTypologyBlock.o Permutation.o SquareMatrix.o TreeTemplate.o TreeTemplatesSet.o nxsassumptionsblock.o nxsblock.o nxscharactersblock.o nxsdatablock.o nxsdistancedatum.o nxsdistancesblock.o nxsexception.o nxsmultiformat.o nxspublicblocks.o nxsreader.o nxssetreader.o nxsstring.o nxstaxablock.o nxstoken.o nxstreesblock.o nxsunalignedblock.o

	$(CC) -o ./bin/Release/BayesLang ./obj/Release/main.o ./obj/Release/Feature.o ./obj/Release/FeatureRateMatricesSet.o ./obj/Release/FeatureRateMatrix.o ./obj/Release/FeatureValuesMatrix.o ./obj/Release/FeaturesSet.o ./obj/Release/InfoHolder.o ./obj/Release/LangNexusReader.o ./obj/Release/LangNexusToken.o ./obj/Release/Language.o ./obj/Release/LanguageTreeTemplate.o ./obj/Release/LanguageWithFeatures.o ./obj/Release/LanguagesSet.o ./obj/Release/MarkovChain.o ./obj/Release/MetropolisCoupledMCMC.o ./obj/Release/NxsTypologyBlock.o ./obj/Release/Permutation.o ./obj/Release/SquareMatrix.o ./obj/Release/TreeTemplate.o ./obj/Release/TreeTemplatesSet.o ./obj/Release/ncl/nxsassumptionsblock.o ./obj/Release/ncl/nxsblock.o ./obj/Release/ncl/nxscharactersblock.o ./obj/Release/ncl/nxsdatablock.o ./obj/Release/ncl/nxsdistancedatum.o ./obj/Release/ncl/nxsdistancesblock.o ./obj/Release/ncl/nxsexception.o ./obj/Release/ncl/nxsmultiformat.o ./obj/Release/ncl/nxspublicblocks.o ./obj/Release/ncl/nxsreader.o ./obj/Release/ncl/nxssetreader.o ./obj/Release/ncl/nxsstring.o ./obj/Release/ncl/nxstaxablock.o ./obj/Release/ncl/nxstoken.o ./obj/Release/ncl/nxstreesblock.o ./obj/Release/ncl/nxsunalignedblock.o -s -l pthread



main.o: main.cpp

	$(COMPILE) -o ./obj/Release/main.o main.cpp -I . -I ./ncl

Feature.o: Feature.cpp

	$(COMPILE) -o ./obj/Release/Feature.o Feature.cpp -I . -I ./ncl

FeatureRateMatricesSet.o: FeatureRateMatricesSet.cpp

	$(COMPILE) -o ./obj/Release/FeatureRateMatricesSet.o FeatureRateMatricesSet.cpp -I . -I ./ncl


FeatureRateMatrix.o: FeatureRateMatrix.cpp

	$(COMPILE) -o ./obj/Release/FeatureRateMatrix.o FeatureRateMatrix.cpp -I . -I ./ncl


FeatureValuesMatrix.o: FeatureValuesMatrix.cpp

	$(COMPILE) -o ./obj/Release/FeatureValuesMatrix.o FeatureValuesMatrix.cpp -I . -I ./ncl


FeaturesSet.o: FeaturesSet.cpp

	$(COMPILE) -o ./obj/Release/FeaturesSet.o FeaturesSet.cpp -I . -I ./ncl


InfoHolder.o: InfoHolder.cpp

	$(COMPILE) -o ./obj/Release/InfoHolder.o InfoHolder.cpp -I . -I ./ncl


LangNexusReader.o: LangNexusReader.cpp

	$(COMPILE) -o ./obj/Release/LangNexusReader.o LangNexusReader.cpp -I . -I ./ncl


LangNexusToken.o: LangNexusToken.cpp

	$(COMPILE) -o ./obj/Release/LangNexusToken.o LangNexusToken.cpp -I . -I ./ncl


Language.o: Language.cpp

	$(COMPILE) -o ./obj/Release/Language.o Language.cpp -I . -I ./ncl


LanguageTreeTemplate.o: LanguageTreeTemplate.cpp

	$(COMPILE) -o ./obj/Release/LanguageTreeTemplate.o LanguageTreeTemplate.cpp -I . -I ./ncl


LanguageWithFeatures.o: LanguageWithFeatures.cpp

	$(COMPILE) -o ./obj/Release/LanguageWithFeatures.o LanguageWithFeatures.cpp -I . -I ./ncl


LanguagesSet.o: LanguagesSet.cpp

	$(COMPILE) -o ./obj/Release/LanguagesSet.o LanguagesSet.cpp -I . -I ./ncl


MarkovChain.o: MarkovChain.cpp

	$(COMPILE) -o ./obj/Release/MarkovChain.o MarkovChain.cpp -I . -I ./ncl


MetropolisCoupledMCMC.o: MetropolisCoupledMCMC.cpp

	$(COMPILE) -o ./obj/Release/MetropolisCoupledMCMC.o MetropolisCoupledMCMC.cpp -I . -I ./ncl


NxsTypologyBlock.o: NxsTypologyBlock.cpp

	$(COMPILE) -o ./obj/Release/NxsTypologyBlock.o NxsTypologyBlock.cpp -I . -I ./ncl


Permutation.o: Permutation.cpp

	$(COMPILE) -o ./obj/Release/Permutation.o Permutation.cpp -I . -I ./ncl


SquareMatrix.o: SquareMatrix.cpp

	$(COMPILE) -o ./obj/Release/SquareMatrix.o SquareMatrix.cpp -I . -I ./ncl


TreeTemplate.o: TreeTemplate.cpp

	$(COMPILE) -o ./obj/Release/TreeTemplate.o TreeTemplate.cpp -I . -I ./ncl


TreeTemplatesSet.o: TreeTemplatesSet.cpp

	$(COMPILE) -o ./obj/Release/TreeTemplatesSet.o TreeTemplatesSet.cpp -I . -I ./ncl


nxsassumptionsblock.o: ./ncl/nxsassumptionsblock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsassumptionsblock.o ./ncl/nxsassumptionsblock.cpp -I . -I ./ncl


nxsblock.o: ./ncl/nxsblock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsblock.o ./ncl/nxsblock.cpp -I . -I ./ncl


nxscharactersblock.o: ./ncl/nxscharactersblock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxscharactersblock.o ./ncl/nxscharactersblock.cpp -I . -I ./ncl


nxsdatablock.o: ./ncl/nxsdatablock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsdatablock.o ./ncl/nxsdatablock.cpp -I . -I ./ncl


nxsdistancedatum.o: ./ncl/nxsdistancedatum.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsdistancedatum.o ./ncl/nxsdistancedatum.cpp -I . -I ./ncl


nxsdistancesblock.o: ./ncl/nxsdistancesblock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsdistancesblock.o ./ncl/nxsdistancesblock.cpp -I . -I ./ncl


nxsexception.o: ./ncl/nxsexception.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsexception.o ./ncl/nxsexception.cpp -I . -I ./ncl


nxsmultiformat.o: ./ncl/nxsmultiformat.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsmultiformat.o ./ncl/nxsmultiformat.cpp -I . -I ./ncl


nxspublicblocks.o: ./ncl/nxspublicblocks.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxspublicblocks.o ./ncl/nxspublicblocks.cpp -I . -I ./ncl


nxsreader.o: ./ncl/nxsreader.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsreader.o ./ncl/nxsreader.cpp -I . -I ./ncl


nxssetreader.o: ./ncl/nxssetreader.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxssetreader.o ./ncl/nxssetreader.cpp -I . -I ./ncl


nxsstring.o: ./ncl/nxsstring.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsstring.o ./ncl/nxsstring.cpp -I . -I ./ncl


nxstaxablock.o: ./ncl/nxstaxablock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxstaxablock.o ./ncl/nxstaxablock.cpp -I . -I ./ncl


nxstoken.o: ./ncl/nxstoken.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxstoken.o ./ncl/nxstoken.cpp -I . -I ./ncl


nxstreesblock.o: ./ncl/nxstreesblock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxstreesblock.o ./ncl/nxstreesblock.cpp -I . -I ./ncl


nxsunalignedblock.o: ./ncl/nxsunalignedblock.cpp

	$(COMPILE) -o ./obj/Release/ncl/nxsunalignedblock.o ./ncl/nxsunalignedblock.cpp -I . -I ./ncl

