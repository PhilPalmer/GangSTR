/*
Copyright (C) 2017 Melissa Gymrek <mgymrek@ucsd.edu>
and Nima Mousavi (mousavi@ucsd.edu)

This file is part of GangSTR.

GangSTR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GangSTR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GangSTR.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <getopt.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>

//#include "src/bam_reader.h"
#include "src/bam_info_extract.h"
#include "src/bam_io.h"
#include "src/common.h"
#include "src/genotyper.h"
#include "src/options.h"
#include "src/ref_genome.h"
#include "src/region_reader.h"
#include "src/stringops.h"
#include "src/vcf_writer.h"

using namespace std;

void show_help() {
  Options options;
  std::stringstream help_msg;
  help_msg << "\nUsage: GangSTR [OPTIONS] "
	   << "--bam <file1[,file2,...]> "
	   << "--ref <reference.fa> "
	   << "--regions <regions.bed> "
	   << "--out <outprefix> "
	   << "\n\n Required options:\n"
	   << "\t" << "--bam         <file.bam>      " << "\t" << "BAM input file" << "\n"
	   << "\t" << "--ref         <genome.fa>     " << "\t" << "FASTA file for the reference genome" << "\n"
	   << "\t" << "--regions     <regions.bed>   " << "\t" << "BED file containing TR coordinates" << "\n"
	   << "\t" << "--out         <outprefix>     " << "\t" << "Prefix to name output files" << "\n"
	   << "\n Additional general options:\n"
	   << "\t" << "--genomewide                  " << "\t" << "Genome-wide mode" << "\n"
	   << "\n Options for different sequencing settings\n"
	   << "\t" << "--readlength  <int>           " << "\t" << "Read length. Default: " << options.read_len << "\n"
	   << "\t" << "--coverage    <float>         " << "\t" << "Average coverage. must be set for exome/targeted data. Default: " << options.coverage << "\n"
	   << "\t" << "--nonuniform                  " << "\t" << "Indicate whether data has non-uniform coverage (i.e., exome)" << "\n"
	   << "\n Advanced paramters for likelihood model:\n"
	   << "\t" << "--frrweight   <float>         " << "\t" << "Weight for FRR reads. Default: " << options.frr_weight << "\n"
	   << "\t" << "--enclweight  <float>         " << "\t" << "Weight for enclosing reads. Default: " << options.enclosing_weight << "\n"
	   << "\t" << "--spanweight  <float>         " << "\t" << "Weight for spanning reads. Default: " << options.spanning_weight << "\n"
	   << "\t" << "--flankweight <float>         " << "\t" << "Weight for flanking reads. Default: " << options.flanking_weight << "\n"
	   << "\t" << "--ploidy      <int>           " << "\t" << "Indicate whether data is haploid (1) or diploid (2). Default: " << options.ploidy << "\n"
	   << "\t" << "--useofftarget               " << "\t" << "Use off target regions included in the BAM file." << "\n"
	   << "\t" << "--insertmean  <float>         " << "\t" << "Fragment length mean. Default: " << options.dist_mean << "\n"
	   << "\t" << "--insertsdev  <float>         " << "\t" << "Fragment length standard deviation. Default: " << options.dist_sdev << "\n"
	   << "\t" << "--insertmax   <float>         " << "\t" << "Maximum insert size. Default " << options.dist_max << "\n"
	   << "\t" << "--read-prob-mode              " << "\t" << "Use only read probability (ignore class probability)" << "\n"
	   << "\t" << "--numbstrap   <int>           " << "\t" << "Number of bootstrap samples. Default: " << options.num_boot_samp << "\n"
	   << "\n Parameters for local realignment:\n"
	   << "\t" << "--minscore    <int>           " << "\t" << "Minimum alignment score (out of 100). Default: " << options.min_score << "\n"
	   << "\t" << "--minmatch    <int>           " << "\t" << "Minimum number of matching basepairs on each end of enclosing reads. Default:L " << options.min_match<< "\n"
	   << "\n Stutter model parameters:\n"
	   << "\t" << "--stutterup   <float>         " << "\t" << "Stutter insertion probability. Default: " << options.stutter_up << "\n"
	   << "\t" << "--stutterdown <float>         " << "\t" << "Stutter deletion probability. Default: " << options.stutter_down << "\n"
	   << "\t" << "--stutterprob <float>         " << "\t" << "Stutter step size parameter. Default: " << options.stutter_p << "\n"
	   << "\n Parameters for more detailed info about each locus:\n"
	   << "\t" << "--output-bootstraps           " << "\t" << "Output file with bootstrap samples" << "\n"
	   << "\t" << "--output-readinfo             " << "\t" << "Output read class info (for debugging)" << "\n"
	   << "\n Additional optional paramters:\n"
	   << "\t" << "-h,--help                     " << "\t" << "display this help screen" << "\n"
	   << "\t" << "--seed                        " << "\t" << "Random number generator initial seed" << "\n"
	   << "\t" << "-v,--verbose                  " << "\t" << "Print out useful progress messages" << "\n"
	   << "\t" << "--very                        " << "\t" << "Print out more detailed progress messages for debugging" << "\n"
	   << "\t" << "--version                     " << "\t" << "Print out the version of this software.\n"
	   << "\n\nThis program takes in aligned reads in BAM format\n"
	   << "and outputs estimated genotypes at each TR in VCF format.\n\n";
  cerr << help_msg.str();
  exit(1);
}

void parse_commandline_options(int argc, char* argv[], Options* options) {
  enum LONG_OPTIONS {
    OPT_BAMFILES,
    OPT_REFFA,
    OPT_REGIONS,
    OPT_OUT,
    OPT_HELP,
    OPT_WFRR,
    OPT_WENCLOSE,
    OPT_WSPAN,
    OPT_WFLANK,
    OPT_GWIDE,
    OPT_PLOIDY,
    OPT_READLEN,
    OPT_COVERAGE,
    OPT_USEOFF,
    OPT_NONUNIF,
    OPT_INSMEAN,
    OPT_INSSDEV,
    OPT_INSMAX,
    OPT_MINSCORE,
    OPT_MINMATCH,
    OPT_STUTUP,
    OPT_STUTDW,
    OPT_STUTPR,
    OPT_NBSTRAP,
    OPT_RDPROB,
    OPT_OUTBS,
    OPT_OUTREADINFO,
    OPT_SEED,
    OPT_VERBOSE,
    OPT_VERYVERBOSE,
    OPT_VERSION,
  };
  static struct option long_options[] = {
    {"bam",         required_argument,  NULL, OPT_BAMFILES},
    {"ref",         required_argument,  NULL, OPT_REFFA},
    {"regions",     required_argument,  NULL, OPT_REGIONS},
    {"out",         required_argument,  NULL, OPT_OUT},
    {"help",        no_argument,        NULL, OPT_HELP},
    {"frrweight",   required_argument,  NULL, OPT_WFRR},      // TODO tried using optional_argument, but it causes segmentation faults
    {"enclweight",  required_argument,  NULL, OPT_WENCLOSE},
    {"spanweight",  required_argument,  NULL, OPT_WSPAN},
    {"flankweight", required_argument,  NULL, OPT_WFLANK},
    {"genomewide",  no_argument, NULL, OPT_GWIDE},
    {"ploidy",      required_argument,  NULL, OPT_PLOIDY},
    {"readlength",  required_argument,  NULL, OPT_READLEN},
    {"coverage",    required_argument,  NULL, OPT_COVERAGE},
    {"nonuniform",  no_argument,  NULL, OPT_NONUNIF},
    {"useofftarget",no_argument,  NULL, OPT_USEOFF},
    {"insertmean",  required_argument,  NULL, OPT_INSMEAN},
    {"insertsdev",  required_argument,  NULL, OPT_INSSDEV},
    {"insertmax",   required_argument,  NULL, OPT_INSMAX},
    {"minscore",    required_argument,  NULL, OPT_MINSCORE},
    {"minmatch",    required_argument,  NULL, OPT_MINMATCH},
    {"stutterup",   required_argument,  NULL, OPT_STUTUP},
    {"stutterdown", required_argument,  NULL, OPT_STUTDW},
    {"stutterprob", required_argument,  NULL, OPT_STUTPR},
    {"numbstrap",   required_argument,  NULL, OPT_NBSTRAP},
    {"read-prob-mode",   no_argument,  NULL, OPT_RDPROB},
    {"output-bootstraps", no_argument,      NULL, OPT_OUTBS},
    {"output-readinfo", no_argument,        NULL, OPT_OUTREADINFO},
    {"seed",        required_argument,  NULL, OPT_SEED},
    {"verbose",     no_argument,        NULL, OPT_VERBOSE},
    {"very",  no_argument, NULL, OPT_VERYVERBOSE},
    {"version",     no_argument,        NULL, OPT_VERSION},
    {NULL,          no_argument,        NULL, 0},
  };
  int ch;
  int option_index = 0;
  ch = getopt_long(argc, argv, "hv?",
                   long_options, &option_index);
  while (ch != -1) {
    switch (ch) {
    case OPT_BAMFILES:
      options->bamfiles.clear();
      split_by_delim(optarg, ',', options->bamfiles);
      break;
    case OPT_REFFA:
      options->reffa = optarg;
      break;
    case OPT_REGIONS:
      options->regionsfile = optarg;
      break;
    case OPT_OUT:
      options->outprefix = optarg;
      break;
    case OPT_HELP:
    case 'h':
      show_help();
    case OPT_WFRR:
      options->frr_weight = atof(optarg);
      break;
    case OPT_WENCLOSE:
      options->enclosing_weight = atof(optarg);
      break;
    case OPT_WSPAN:
      options->spanning_weight = atof(optarg);
      break;
    case OPT_WFLANK:
      options->flanking_weight = atof(optarg);
      break;
    case OPT_GWIDE:
      options->genome_wide = true;
      break;
    case OPT_PLOIDY:
      options->ploidy = atoi(optarg);
      break;
    case OPT_READLEN:
      options->read_len = atoi(optarg);
      break;
    case OPT_COVERAGE:
      options->coverage = atof(optarg);
      break;
    case OPT_NONUNIF:
      options->use_cov = false;
      break;
    case OPT_USEOFF:
      options->use_off = true;
      break;
    case OPT_INSMEAN:
      options->dist_mean = atoi(optarg);
      options->dist_man_set = true;
      break;
    case OPT_INSSDEV:
      options->dist_sdev = atoi(optarg);
      options->dist_man_set = true;
      break;
    case OPT_INSMAX:
      options->dist_max = atoi(optarg);
      break;
    case OPT_MINSCORE:
      options->min_score = atoi(optarg);
      break;
    case OPT_MINMATCH:
      options->min_match = atoi(optarg);
      break;
    case OPT_STUTUP:
      options->stutter_up = atof(optarg);
      break;
    case OPT_STUTDW:
      options->stutter_down = atof(optarg);
      break;
    case OPT_STUTPR:
      options->stutter_p = atof(optarg);
      break;
    case OPT_NBSTRAP:
      options->num_boot_samp = atoi(optarg);
      break;
    case OPT_OUTBS:
      options->output_bootstrap++;
      break;
    case OPT_RDPROB:
      options->read_prob_mode++;
      break;
    case OPT_OUTREADINFO:
      options->output_readinfo++;
      break;
    case OPT_SEED:
      options->seed = atoi(optarg);
      break;
    case OPT_VERBOSE:
    case 'v':
      options->verbose++;
      break;
    case OPT_VERYVERBOSE:
      options->very_verbose++;
      break;
    case OPT_VERSION:
      cerr << _GIT_VERSION << endl;
      exit(0);
    case '?':
      show_help();
    default:
      show_help();
    };
    ch = getopt_long(argc, argv, "hv?",
		     long_options, &option_index);
  };
  // Leftover arguments are errors
  if (optind < argc) {
    PrintMessageDieOnError("Unnecessary leftover arguments", M_ERROR);
  }
  // Perform other error checking
  if (options->bamfiles.empty()) {
    PrintMessageDieOnError("No --bam files specified", M_ERROR);
  }
  if (options->regionsfile.empty()) {
    PrintMessageDieOnError("No --regions option specified", M_ERROR);
  }
  if (options->reffa.empty()) {
    PrintMessageDieOnError("No --ref option specified", M_ERROR);
  }
  if (options->outprefix.empty()) {
    PrintMessageDieOnError("No --out option specified", M_ERROR);
  }
  if (options->min_match < 0 or (options->read_len != -1 and options->min_match > options->read_len)){
    PrintMessageDieOnError("--minmatch parameter must be in (0, read_len) range", M_ERROR);
  }
  if (options->min_score < 0 and options->min_score > 100){
    PrintMessageDieOnError("--min_score parameter must be in (0, 100) range", M_ERROR);
  }
  
}

int main(int argc, char* argv[]) {
  // Set up
  Options options;
  parse_commandline_options(argc, argv, &options);
  stringstream full_command_ss;
  full_command_ss << "GangSTR-" << _GIT_VERSION;
  for (int i = 1; i < argc; i++) {
    full_command_ss << " " << argv[i];
  }
  std::string full_command = full_command_ss.str();
  RegionReader region_reader(options.regionsfile);
  Locus locus;
  int merge_type = BamCramMultiReader::ORDER_ALNS_BY_FILE;
  BamCramMultiReader bamreader(options.bamfiles, options.reffa, merge_type);


  // Extract information from bam file (read length, insert size distribution, ..)
  int32_t read_len;
  double mean, std_dev, coverage;
  BamInfoExtract bam_info(&options, &bamreader, &region_reader);
  if (options.genome_wide == true){
    PrintMessageDieOnError("\tRunning in whole genome mode", M_PROGRESS);
  }
  if(options.read_len == -1){  // if read_len wasn't set, we need to extract from bam.
    if (options.verbose) {
      PrintMessageDieOnError("\tExtracting read length", M_PROGRESS);
    }
    if(!bam_info.GetReadLen(&read_len)){
      PrintMessageDieOnError("No Locus contains enough reads to extract read length. (Possible mismatch in chromosome names)", M_ERROR);
    }
    options.read_len = read_len;
    options.realignment_flanklen = read_len;
    if (options.verbose) {
      stringstream ss;
      ss << "\tRead_Length=" << read_len;
      PrintMessageDieOnError(ss.str(), M_PROGRESS);
    }
  }
  region_reader.Reset();
  if(options.dist_mean == -1 or options.dist_sdev == -1 or options.coverage == -1){
    if (options.verbose) {
      PrintMessageDieOnError("\tComputing insert size distribution and/or coverage", M_PROGRESS);
    }
    if(!bam_info.GetInsertSizeDistribution(&mean, &std_dev, &coverage)){
      PrintMessageDieOnError("No Locus contains enough reads to extract insert size mean and standard deviation. (Possible mismatch in chromosome names)", M_ERROR);
    }
    if (options.dist_mean == -1 or options.dist_sdev == -1){
      options.dist_mean = mean;
      options.dist_sdev = std_dev;
    }
    if (options.use_cov){
      if (options.coverage == -1){
	if (coverage < 10){
	  PrintMessageDieOnError("Low coverage or targeted data. Please set coverage manually.", M_ERROR);
	}
	options.coverage = coverage;
      }
      else{
	coverage = options.coverage;
      }
    }
    else{
      stringstream ss;
      ss << "\tNonUniform mode selected. Not using coverage in compuations.";
      PrintMessageDieOnError(ss.str(), M_PROGRESS);
    }
    if (options.verbose) {
      stringstream ss;
      ss << "\tMean=" << mean << " SD=" << std_dev<<" Cov="<<coverage;
      PrintMessageDieOnError(ss.str(), M_PROGRESS);
    }
  }
  if (options.dist_max == -1){
    options.dist_max = options.dist_mean + options.dist_sdev * 3;
  }

  // Process each region
  region_reader.Reset();
  RefGenome refgenome(options.reffa);
  VCFWriter vcfwriter(options.outprefix + ".vcf", full_command);
  Genotyper genotyper(refgenome, options);
  stringstream ss;
  while (region_reader.GetNextRegion(&locus)) {
    ss.str("");
    ss.clear();
    ss << "Processing " << locus.chrom << ":" << locus.start;
    if (options.use_off == true){
      locus.offtarget_share = 1.0;
    }
    else{
      locus.offtarget_share = 0.0;
    }
    
    /*
    for (std::vector<GenomeRegion>::iterator it = locus.offtarget_regions.begin();
	 it != locus.offtarget_regions.end(); it++){
      cerr << it->chrom << " " << it->start << " " << it->end << endl;
    }
    */
    PrintMessageDieOnError(ss.str(), M_PROGRESS);
    locus.insert_size_mean = options.dist_mean;
    locus.insert_size_stddev = options.dist_sdev = std_dev;

    if (genotyper.ProcessLocus(&bamreader, &locus)) {
      vcfwriter.WriteRecord(locus);
    }
    locus.Reset();
  };
}
