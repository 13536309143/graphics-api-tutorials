#pragma once
namespace course {
// Each id dispatches to an actual GPU/API experiment; chapters may share an experiment.
int nativeLabMain(int argc, char** argv, int id);
} // namespace course
