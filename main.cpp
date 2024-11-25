// xmovse00 Movsesian Lilit
// xkorni03 Kornieiev Pavlo

#include "simlib.h"

// Facility representing a machine that sorts tomatoes by size.
// Capacity: 1 tomato at a time. Processing time: 0.5 seconds.
Facility tomatoSizeSorter("tomatoSizeSorter");

// Facility representing a machine that separates defective tomatoes.
// Capacity: 1 tomato at a time. Processing time: 0.5 seconds.
Facility tomatoDefectSorter("tomatoDefectSorter");

// Facility representing a quality control worker who manually inspects tomatoes.
// Capacity: 1 tomato at a time. Processing time: 0.5 seconds.
Facility qualityControlWorker("qualityControlWorker");

// Store representing 2 machines that fills jars with tomatoes.
// Capacity: 1 jar at a time. Processing time: 1 second.
Store tomatoJarFiller("tomatoJarFiller", 2);

// Store representing 2 machines that fills jars with brine.
// Capacity: 1 jar at a time. Processing time: 3 seconds.
Store brineJarFiller("brineJarFiller", 2);

// Store representing 2 machines that closes the jars with lids.
// Capacity: 1 jar at a time. Processing time: 3 seconds.
Store lidCloser("lidCloser", 2);

// Store representing 9 sterilization machines used to sterilize jars.
// Capacity: 100 jars per batch. Processing time: 2400 seconds.
Store sterilizationMachine("sterilizationMachine", 9);

// Store representing 10 machines that applies labels to the jars.
// Capacity: 1 jar at a time. Processing time: 2 seconds.
Store labelApplicator("labelApplicator", 10);

// Store representing 10 machines that prints production and expiration dates on jars.
// Capacity: 1 jar at a time. Processing time: 2 seconds.
Store datePrinter("datePrinter", 10);

// Queue to manage jars waiting to be processed in the sterilization machine.
Queue sterilizationQueue;

const double SIZE_SORTER_TIME = 0.5;
const double DEFECT_SORTER_TIME = 0.5;
const double QUALITY_CONTROL_TIME = 0.5;
const double JAR_FILLER_TIME = 1.0; // For 1 tomato
const double BRINE_FILLER_TIME = 3.0;
const double LID_CLOSER_TIME = 3.0;
const double STERILIZATION_TIME = 2400.0; // 40 minutes
const double LABEL_APPLICATOR_TIME = 2.0;
const double DATE_PRINTER_TIME = 2.0;
const int TOMATOES_PER_JAR = 5; // Number of tomatoes per jar
const int STERILIZATION_CAPACITY = 100; // Number of jars per sterilization batch

Stat tomatoProcessingTime("Time to Process One Tomato Before Filling");
Stat jarProcessingTime("Time to Process One Jar After Filling Before Sterilization");
Stat sterilizedJarProcessingTime("Time to Process One Jar After Sterilization");

int totalTomatoesGenerated = 0;
int totalTomatoesRejectedSize = 0;
int totalTomatoesRejectedDefects = 0;
int totalTomatoesRejectedWorker = 0;
int totalJarsFilled = 0;
int totalJarsSterilized = 0;
int totalJarsLabelApplied = 0;
int totalJarsDatePrinted = 0;

int tomatoesInCurrentJar = 0;



class SterilizedJar : public Process {
    void Behavior() override {
        double startTime = Time; 

        // Apply labels
        Enter(labelApplicator, 1);
        Wait(LABEL_APPLICATOR_TIME);
        Leave(labelApplicator, 1);
        totalJarsLabelApplied++;

        // Print dates
        Enter(datePrinter, 1);
        Wait(DATE_PRINTER_TIME);
        Leave(datePrinter, 1);
        totalJarsDatePrinted++;

        sterilizedJarProcessingTime(Time - startTime);
    }
};


class Jar : public Process {
    void Behavior() override {
        double startTime = Time; 
        
        // Fill brine
        Enter(brineJarFiller, 1);
        Wait(BRINE_FILLER_TIME);
        Leave(brineJarFiller, 1);

        // Close lid
        Enter(lidCloser, 1);
        Wait(LID_CLOSER_TIME);
        Leave(lidCloser, 1);

        Into(sterilizationQueue);
        
        if (sterilizationQueue.Length() >= STERILIZATION_CAPACITY ) { 
            // Free 100 jars from queue
            for (int i = 0; i < STERILIZATION_CAPACITY; i++) {
                sterilizationQueue.GetFirst()->Activate();
            }
            Enter(sterilizationMachine, 1);
            Wait(STERILIZATION_TIME);
            Leave(sterilizationMachine, 1);
            for (int i = 0; i < STERILIZATION_CAPACITY; i++) {
                totalJarsSterilized++;
                (new SterilizedJar)->Activate();
            }

        } else {
            // Passivate the jar process until the sterilization queue has 100 jars
            Passivate();
        }

        jarProcessingTime(Time - startTime);       
    }
};


class Tomato : public Process {
    void Behavior() override {
        
        double startTime = Time;

        // Size sorting
        Seize(tomatoSizeSorter);
        Wait(SIZE_SORTER_TIME);
        Release(tomatoSizeSorter);
        if (Random() < 0.05) { // 5% sorted out
            totalTomatoesRejectedSize++;
            tomatoProcessingTime(Time - startTime);
            return;
        }

        // Sort defective
        Seize(tomatoDefectSorter);
        Wait(DEFECT_SORTER_TIME);
        Release(tomatoDefectSorter);
        if (Random() < 0.05) { // 5% sorted out
            totalTomatoesRejectedDefects++;
            tomatoProcessingTime(Time - startTime);
            return;
        }

        // Worker control
        Seize(qualityControlWorker);
        Wait(QUALITY_CONTROL_TIME);
        Release(qualityControlWorker);
        if (Random() < 0.02) { // 2% sorted out
            totalTomatoesRejectedWorker++;
            tomatoProcessingTime(Time - startTime);
            return;
        }

        // Fill tomato
        Enter(tomatoJarFiller, 1);
        Wait(JAR_FILLER_TIME);
        Leave(tomatoJarFiller, 1);

        tomatoProcessingTime(Time - startTime);

        tomatoesInCurrentJar++;
        if (tomatoesInCurrentJar == TOMATOES_PER_JAR) {
            tomatoesInCurrentJar = 0;
            totalJarsFilled++;
            (new Jar)->Activate();
        }

    }
};


class TomatoGenerator : public Event {
    void Behavior() override {
        totalTomatoesGenerated++;
        (new Tomato)->Activate();
        Activate(Time + 0.5);
    }
};


int main() {
    SetOutput("output.txt");

    // 24 hours simulation
    Init(0, 86400);
    
    (new TomatoGenerator)->Activate();

    Run();
    
    tomatoSizeSorter.Output();
    tomatoDefectSorter.Output();
    qualityControlWorker.Output();
    tomatoJarFiller.Output();
    brineJarFiller.Output();
    lidCloser.Output();
    sterilizationMachine.Output();
    labelApplicator.Output();
    datePrinter.Output();
    
    tomatoProcessingTime.Output();
    jarProcessingTime.Output();
    sterilizedJarProcessingTime.Output();

    Print("Total tomatoes generated: %d\n", totalTomatoesGenerated);
    Print("Number of tomatoes rejected due to size: %d\n", totalTomatoesRejectedSize);
    Print("Number of tomatoes rejected due to defects: %d\n", totalTomatoesRejectedDefects);
    Print("Number of tomatoes rejected by quality control: %d\n", totalTomatoesRejectedWorker);
    Print("Total jars filled: %d\n", totalJarsFilled);
    Print("Total jars sterilized: %d\n", totalJarsSterilized);
    Print("Total jars label applied: %d\n", totalJarsLabelApplied);
    Print("Total jars date printed: %d\n", totalJarsDatePrinted);
    return 0;
}