clear;
clc;
close all;

data = importdata('dataP1K100-200.csv');
prodNum = 1;
consNum = [1 2 4 8 16 32 64 128];
n = length(consNum);

stats = zeros(n,6);
stats(:,1) = consNum';

for i = 1:n
    
    dataTemp = data(i,:);
    dataTemp = dataTemp(4801:end-200);
    %dataTemp = dataTemp(dataTemp<100);
       
    stats(i,2) = mean(dataTemp);
    stats(i,3) = std(dataTemp);
    stats(i,4) = median(dataTemp);
    stats(i,5) = min(dataTemp);
    stats(i,6) = max(dataTemp);
    
    figure();
    hold on;
    histogram(dataTemp);
    title(['#Producers=' num2str(prodNum) ' - #Consumers=' num2str(consNum(i))]);
    xlabel('Time(us)');
    ylabel('#Jobs');
    %saveas(gcf,sprintf('Cons%d.png', stats(i,1)));

    figure();
    hold on;
    plot(dataTemp,'.');
    title(['#Producers=' num2str(prodNum) ' - #Consumers=' num2str(consNum(i))]);
    

end

figure();
hold on;
plot(consNum, stats(:,2));
scatter(consNum, stats(:,2));
xlabel('#Consumers');
ylabel('Average Waiting Time (us)');
%saveas(gcf,'Average Waiting Time.png');
