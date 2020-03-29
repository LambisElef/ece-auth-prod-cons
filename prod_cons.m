clear;
clc;
close all;

data = importdata('dataP6K10-20.csv');
prodNum = 6;
consNum = [1 2 4 8 16 32 64 128];
n = length(consNum);

dataMean = zeros(n,1);
for i = 1:n
    dataTemp = data(i,:);
    dataTemp = dataTemp(11:end);
    dataTemp = dataTemp(dataTemp<100);
    figure();
    hold on;
    histogram(dataTemp,80);
    title(['#Producers=' num2str(prodNum) ' - #Consumers=' num2str(consNum(i))]);
    xlabel('Time(us)');
    ylabel('#Jobs');
    
    dataMean(i) = mean(dataTemp);
end

figure();
hold on;
plot(consNum, dataMean);
scatter(consNum, dataMean);
xlabel('#Consumers');
ylabel('Average Waiting Time (us)');