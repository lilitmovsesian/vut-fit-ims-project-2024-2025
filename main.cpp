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

// Facility representing a machine that fills jars with tomatoes.
// Capacity: 1 jar at a time. Processing time: 1 second.
Facility tomatoJarFiller("tomatoJarFiller");

// Facility representing a machine that fills jars with brine.
// Capacity: 1 jar at a time. Processing time: 3 seconds.
Facility brineJarFiller("brineJarFiller");

// Facility representing a machine that closes the jars with lids.
// Capacity: 1 jar at a time. Processing time: 3 seconds.
Facility lidCloser("lidCloser");

// Facility representing a sterilization machine used to sterilize jars.
// Capacity: 50 jars per batch. Processing time: 2400 seconds.
Facility sterilizationMachine("sterilizationMachine");

// Facility representing a machine that applies labels to the jars.
// Capacity: 1 jar at a time. Processing time: 3 seconds.
Facility labelApplicator("labelApplicator");

// Facility representing a machine that prints production and expiration dates on jars.
// Capacity: 1 jar at a time. Processing time: 3 seconds.
Facility datePrinter("datePrinter");

// Queue to manage jars waiting to be processed in the sterilization machine.
Queue sterilizationQueue;

const double SIZE_SORTER_TIME = 0.5;
const double DEFECT_SORTER_TIME = 0.5;
const double QUALITY_CONTROL_TIME = 0.5;
const double JAR_FILLER_TIME = 1.0; // For 1 tomato
const double BRINE_FILLER_TIME = 3.0;
const double LID_CLOSER_TIME = 3.0;
const double STERILIZATION_TIME = 2400.0; // 40 minutes
const double LABEL_APPLICATOR_TIME = 3.0;
const double DATE_PRINTER_TIME = 3.0;
const int TOMATOES_PER_JAR = 5; // Number of tomatoes per jar
const int STERILIZATION_CAPACITY = 50; // Number of jars per sterilization batch

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
        Seize(labelApplicator);
        Wait(LABEL_APPLICATOR_TIME);
        Release(labelApplicator);
        totalJarsLabelApplied++;

        // Print dates
        Seize(datePrinter);
        Wait(DATE_PRINTER_TIME);
        Release(datePrinter);
        totalJarsDatePrinted++;

        sterilizedJarProcessingTime(Time - startTime);
    }
};

class SterilizedJarGenerator : public Event {
    void Behavior() override {
        (new SterilizedJar)->Activate();
    }
};

class Jar : public Process {
    void Behavior() override {
        double startTime = Time; 
        
        // Fill brine
        Seize(brineJarFiller);
        Wait(BRINE_FILLER_TIME);
        Release(brineJarFiller);

        // Close lid
        Seize(lidCloser);
        Wait(LID_CLOSER_TIME);
        Release(lidCloser);

        Into(sterilizationQueue);
        
        if (sterilizationQueue.Length() >= STERILIZATION_CAPACITY ) { 
            // Free 50 jars from queue
            for (int i = 0; i < STERILIZATION_CAPACITY; i++) {
                sterilizationQueue.GetFirst()->Activate();
                totalJarsSterilized++;
            }
            Seize(sterilizationMachine);
            Wait(STERILIZATION_TIME);
            Release(sterilizationMachine);
            for (int i = 0; i < STERILIZATION_CAPACITY; i++) {
                (new SterilizedJarGenerator)->Activate();
            }

        } else {
            // Passivate the jar process until the sterilization queue has 50 jars
            Passivate();
        }

        jarProcessingTime(Time - startTime);       
    }
};


class JarGenerator : public Event {
    void Behavior() override {
        (new Jar)->Activate();
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
        Seize(tomatoJarFiller);
        Wait(JAR_FILLER_TIME);
        Release(tomatoJarFiller);

        tomatoProcessingTime(Time - startTime);

        tomatoesInCurrentJar++;
        if (tomatoesInCurrentJar == TOMATOES_PER_JAR) {
            tomatoesInCurrentJar = 0;
            totalJarsFilled++;
            (new JarGenerator)->Activate();
        }

    }
};


class TomatoGenerator : public Event {
    void Behavior() override {
        totalTomatoesGenerated++;
        (new Tomato)->Activate();
        Activate(Time + 1);
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